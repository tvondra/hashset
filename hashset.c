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
	set->hash = 0; /* Initial hash value */

	set->flags |= 0;

	return set;
}

int4hashset_t *
int4hashset_resize(int4hashset_t * set)
{
	int				i;
	int4hashset_t	*new;
	char			*bitmap;
	int32			*values;

	new = int4hashset_allocate(
		set->capacity * 2,
		set->load_factor,
		set->growth_factor,
		set->hashfn_id
	);

	/* Calculate the pointer to the bitmap and values array */
	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->capacity, 8));

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

	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->capacity, 8));

	while (true)
	{
		byte = (position / 8);
		bit = (position % 8);

		/* the item is already used - maybe it's the same value? */
		if (bitmap[byte] & (0x01 << bit))
		{
			/* same value, we're done */
			if (values[position] == value)
				break;

			/* Increment the collision counter */
			set->ncollisions++;

			position = (position + HASHSET_STEP) % set->capacity;
			continue;
		}

		/* found an empty spot, before hitting the value first */
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
	char   *bitmap;
	int32  *values;
	int     num_probes = 0; /* Add a counter for the number of probes */

	if (set->hashfn_id == JENKINS_LOOKUP3_HASHFN_ID)
	{
		hash = hash_bytes_uint32((uint32) value) % set->capacity;
	}
	else if (set->hashfn_id == MURMURHASH32_HASHFN_ID)
	{
		hash = murmurhash32((uint32) value) % set->capacity;
	}
	else if (set->hashfn_id == NAIVE_HASHFN_ID)
	{
		hash = ((uint32) value * 7691 + 4201) % set->capacity;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid hash function ID: \"%d\"", set->hashfn_id)));
	}

	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->capacity, 8));

	while (true)
	{
		byte = (hash / 8);
		bit = (hash % 8);

		/* found an empty slot, value is not there */
		if ((bitmap[byte] & (0x01 << bit)) == 0)
			return false;

		/* is it the same value? */
		if (values[hash] == value)
			return true;

		/* move to the next element */
		hash = (hash + HASHSET_STEP) % set->capacity;

		num_probes++; /* Increment the number of probes */

		/* Check if we have probed all slots */
		if (num_probes >= set->capacity)
			return false; /* Avoid infinite loop */
	}
}

int32 *
int4hashset_extract_sorted_elements(int4hashset_t *set)
{
	/* Allocate memory for the elements array */
	int32 *elements = palloc(set->nelements * sizeof(int32));

	/* Access the data array */
	char *bitmap = set->data;
	int32 *values = (int32 *)(set->data + CEIL_DIV(set->capacity, 8));

	/* Counter for the number of extracted elements */
	int32 nextracted = 0;

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
 * construct an SQL array from a simple C double array
 */
Datum
int32_to_array(FunctionCallInfo fcinfo, int32 *d, int len)
{
	ArrayBuildState *astate = NULL;
	int		 i;

	for (i = 0; i < len; i++)
	{
		/* stash away this field */
		astate = accumArrayResult(astate,
								  Int32GetDatum(d[i]),
								  false,
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
