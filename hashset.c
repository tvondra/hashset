/*
 * hashset.c
 *
 * Copyright (C) Tomas Vondra, 2019
 */

#include "hashset.h"

static int int32_cmp(const void *a, const void *b);

int4hashset_t *
int4hashset_allocate(
	int capacity,
	float4 load_factor,
	float4 growth_factor,
	int hashfn_id
)
{
	Size			len;
	int4hashset_t  *set;
	char		   *ptr;

	/*
	 * Ensure that capacity is not divisible by HASHSET_STEP;
	 * i.e. the step size used in hashset_add_element()
	 * and hashset_contains_element().
	 */
	while (capacity % HASHSET_STEP == 0)
		capacity++;

	len = offsetof(int4hashset_t, data);
	len += CEIL_DIV(capacity, 8);
	len += capacity * sizeof(int32);

	ptr = palloc0(len);
	SET_VARSIZE(ptr, len);

	set = (int4hashset_t *) ptr;

	set->flags = 0;
	set->capacity = capacity;
	set->nelements = 0;
	set->hashfn_id = hashfn_id;
	set->load_factor = load_factor;
	set->growth_factor = growth_factor;
	set->ncollisions = 0;
	set->max_collisions = 0;
	set->hash = 0; /* Initial hash value */
	set->null_element = false; /* No null element initially */

	set->flags |= 0;

	return set;
}

int4hashset_t *
int4hashset_resize(int4hashset_t * set)
{
	int				i;
	int4hashset_t  *new;
	char		   *bitmap = HASHSET_GET_BITMAP(set);
	int32		   *values = HASHSET_GET_VALUES(set);
	int				new_capacity;

	new_capacity = (int)(set->capacity * set->growth_factor);

	/*
	 * If growth factor is too small, new capacity might remain the same as
	 * the old capacity. This can lead to an infinite loop in resizing.
	 * To prevent this, we manually increment the capacity by 1 if new capacity
	 * equals the old capacity.
	 */
	if (new_capacity == set->capacity)
		new_capacity = set->capacity + 1;

	new = int4hashset_allocate(
		new_capacity,
		set->load_factor,
		set->growth_factor,
		set->hashfn_id
	);

	for (i = 0; i < set->capacity; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (bitmap[byte] & (0x01 << bit))
			int4hashset_add_element(new, values[i]);
	}

	return new;
}

int4hashset_t *
int4hashset_add_element(int4hashset_t *set, int32 value)
{
	int		byte;
	int		bit;
	uint32	hash;
	uint32	position;
	char   *bitmap;
	int32  *values;
	int32	current_collisions = 0;

	if (set->nelements > set->capacity * set->load_factor)
		set = int4hashset_resize(set);

	if (set->hashfn_id == JENKINS_LOOKUP3_HASHFN_ID)
	{
		hash = hash_bytes_uint32((uint32) value);
	}
	else if (set->hashfn_id == MURMURHASH32_HASHFN_ID)
	{
		hash = murmurhash32((uint32) value);
	}
	else if (set->hashfn_id == NAIVE_HASHFN_ID)
	{
		hash = ((uint32) value * 7691 + 4201);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid hash function ID: \"%d\"", set->hashfn_id)));
	}

    position = hash % set->capacity;

	bitmap = HASHSET_GET_BITMAP(set);
	values = HASHSET_GET_VALUES(set);

	while (true)
	{
		byte = (position / 8);
		bit = (position % 8);

		/* The item is already used - maybe it's the same value? */
		if (bitmap[byte] & (0x01 << bit))
		{
			/* Same value, we're done */
			if (values[position] == value)
				break;

			/* Increment the collision counter */
			set->ncollisions++;
			current_collisions++;

			if (current_collisions > set->max_collisions)
				set->max_collisions = current_collisions;

			position = (position + HASHSET_STEP) % set->capacity;
			continue;
		}

		/* Found an empty spot, before hitting the value first */
		bitmap[byte] |= (0x01 << bit);
		values[position] = value;

		set->hash ^= hash;

		set->nelements++;

		break;
	}

	return set;
}

bool
int4hashset_contains_element(int4hashset_t *set, int32 value)
{
	int     byte;
	int     bit;
	uint32  hash;
	uint32	position;
	char   *bitmap = HASHSET_GET_BITMAP(set);
	int32  *values = HASHSET_GET_VALUES(set);
	int     num_probes = 0; /* Counter for the number of probes */

	if (set->hashfn_id == JENKINS_LOOKUP3_HASHFN_ID)
	{
		hash = hash_bytes_uint32((uint32) value);
	}
	else if (set->hashfn_id == MURMURHASH32_HASHFN_ID)
	{
		hash = murmurhash32((uint32) value);
	}
	else if (set->hashfn_id == NAIVE_HASHFN_ID)
	{
		hash = ((uint32) value * NAIVE_HASHFN_MULTIPLIER + NAIVE_HASHFN_INCREMENT);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid hash function ID: \"%d\"", set->hashfn_id)));
	}

	position = hash % set->capacity;

	while (true)
	{
		byte = (position / 8);
		bit = (position % 8);

		/* Found an empty slot, value is not there */
		if ((bitmap[byte] & (0x01 << bit)) == 0)
			return false;

		/* Is it the same value? */
		if (values[position] == value)
			return true;

		/* Move to the next element */
		position = (position + HASHSET_STEP) % set->capacity;

		num_probes++; /* Increment the number of probes */

		/* Check if we have probed all slots */
		if (num_probes >= set->capacity)
			return false; /* Avoid infinite loop */
	}
}

int32 *
int4hashset_extract_sorted_elements(int4hashset_t *set)
{
	int32  *elements = palloc(set->nelements * sizeof(int32));
	char   *bitmap = HASHSET_GET_BITMAP(set);
	int32  *values = HASHSET_GET_VALUES(set);
	int32	nextracted = 0;

	/* Iterate through all elements */
	for (int32 i = 0; i < set->capacity; i++)
	{
		int byte = i / 8;
		int bit = i % 8;

		/* Check if the current position is occupied */
		if (bitmap[byte] & (0x01 << bit))
		{
			/* Add the value to the elements array */
			elements[nextracted++] = values[i];
		}
	}

	/* Make sure we extracted the correct number of elements */
	Assert(nextracted == set->nelements);

	/* Sort the elements array */
	qsort(elements, nextracted, sizeof(int32), int32_cmp);

	/* Return the sorted elements array */
	return elements;
}

int4hashset_t *
int4hashset_copy(int4hashset_t *src)
{
	return src;
}

/*
 * hashset_isspace() --- a non-locale-dependent isspace()
 *
 * Identical to array_isspace() in src/backend/utils/adt/arrayfuncs.c.
 * We used to use isspace() for parsing hashset values, but that has
 * undesirable results: a hashset value might be silently interpreted
 * differently depending on the locale setting. So here, we hard-wire
 * the traditional ASCII definition of isspace().
 */
bool
hashset_isspace(char ch)
{
	if (ch == ' ' ||
		ch == '\t' ||
		ch == '\n' ||
		ch == '\r' ||
		ch == '\v' ||
		ch == '\f')
		return true;
	return false;
}

/*
 * Construct an SQL array from a simple C double array
 */
Datum
int32_to_array(FunctionCallInfo fcinfo, int32 *d, int len, bool null_element)
{
	ArrayBuildState *astate = NULL;
	int		 i;

	for (i = 0; i < len; i++)
	{
		/* Stash away this field */
		astate = accumArrayResult(astate,
								  Int32GetDatum(d[i]),
								  false,
								  INT4OID,
								  CurrentMemoryContext);
	}

	if (null_element)
	{
		astate = accumArrayResult(astate,
								  (Datum) 0,
								  true,
								  INT4OID,
								  CurrentMemoryContext);
	}

	PG_RETURN_DATUM(makeArrayResult(astate,
					CurrentMemoryContext));
}

static int
int32_cmp(const void *a, const void *b)
{
	int32 arg1 = *(const int32 *)a;
	int32 arg2 = *(const int32 *)b;

	if (arg1 < arg2) return -1;
	if (arg1 > arg2) return 1;
	return 0;
}
