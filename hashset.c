/*
 * hashset.c
 *
 * Copyright (C) Tomas Vondra, 2019
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>

#include "postgres.h"
#include "libpq/pqformat.h"
#include "nodes/memnodes.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "catalog/pg_type.h"
#include "common/hashfn.h"

PG_MODULE_MAGIC;

/*
 * hashset
 */
typedef struct hashset_t {
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int32		flags;			/* reserved for future use (versioning, ...) */
	int32		maxelements;	/* max number of element we have space for */
	int32		nelements;		/* number of items added to the hashset */
	int32		hashfn_id;		/* ID of the hash function used */
	char		data[FLEXIBLE_ARRAY_MEMBER];
} hashset_t;

static hashset_t *hashset_resize(hashset_t * set);
static hashset_t *hashset_add_element(hashset_t *set, int32 value);
static bool hashset_contains_element(hashset_t *set, int32 value);

static Datum int32_to_array(FunctionCallInfo fcinfo, int32 * d, int len);

#define PG_GETARG_HASHSET(x)	(hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(x))
#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define HASHSET_STEP 13
#define JENKINS_LOOKUP3_HASHFN_ID 1

PG_FUNCTION_INFO_V1(hashset_in);
PG_FUNCTION_INFO_V1(hashset_out);
PG_FUNCTION_INFO_V1(hashset_send);
PG_FUNCTION_INFO_V1(hashset_recv);
PG_FUNCTION_INFO_V1(hashset_add);
PG_FUNCTION_INFO_V1(hashset_contains);
PG_FUNCTION_INFO_V1(hashset_count);
PG_FUNCTION_INFO_V1(hashset_merge);
PG_FUNCTION_INFO_V1(hashset_init);
PG_FUNCTION_INFO_V1(hashset_agg_add_set);
PG_FUNCTION_INFO_V1(hashset_agg_add);
PG_FUNCTION_INFO_V1(hashset_agg_final);
PG_FUNCTION_INFO_V1(hashset_agg_combine);
PG_FUNCTION_INFO_V1(hashset_to_array);
PG_FUNCTION_INFO_V1(hashset_equals);
PG_FUNCTION_INFO_V1(hashset_neq);
PG_FUNCTION_INFO_V1(hashset_hash);
PG_FUNCTION_INFO_V1(hashset_lt);
PG_FUNCTION_INFO_V1(hashset_le);
PG_FUNCTION_INFO_V1(hashset_gt);
PG_FUNCTION_INFO_V1(hashset_ge);
PG_FUNCTION_INFO_V1(hashset_cmp);

Datum hashset_in(PG_FUNCTION_ARGS);
Datum hashset_out(PG_FUNCTION_ARGS);
Datum hashset_send(PG_FUNCTION_ARGS);
Datum hashset_recv(PG_FUNCTION_ARGS);
Datum hashset_add(PG_FUNCTION_ARGS);
Datum hashset_contains(PG_FUNCTION_ARGS);
Datum hashset_count(PG_FUNCTION_ARGS);
Datum hashset_merge(PG_FUNCTION_ARGS);
Datum hashset_init(PG_FUNCTION_ARGS);
Datum hashset_agg_add(PG_FUNCTION_ARGS);
Datum hashset_agg_add_set(PG_FUNCTION_ARGS);
Datum hashset_agg_final(PG_FUNCTION_ARGS);
Datum hashset_agg_combine(PG_FUNCTION_ARGS);
Datum hashset_to_array(PG_FUNCTION_ARGS);
Datum hashset_equals(PG_FUNCTION_ARGS);
Datum hashset_neq(PG_FUNCTION_ARGS);
Datum hashset_hash(PG_FUNCTION_ARGS);
Datum hashset_lt(PG_FUNCTION_ARGS);
Datum hashset_le(PG_FUNCTION_ARGS);
Datum hashset_gt(PG_FUNCTION_ARGS);
Datum hashset_ge(PG_FUNCTION_ARGS);
Datum hashset_cmp(PG_FUNCTION_ARGS);

static hashset_t *
hashset_allocate(int maxelements)
{
	Size		len;
	hashset_t  *set;
	char	   *ptr;

	/*
	 * Ensure that maxelements is not divisible by HASHSET_STEP;
	 * i.e. the step size used in hashset_add_element()
	 * and hashset_contains_element().
	 */
	while (maxelements % HASHSET_STEP == 0)
		maxelements++;

	len = offsetof(hashset_t, data);
	len += CEIL_DIV(maxelements, 8);
	len += maxelements * sizeof(int32);

	ptr = palloc0(len);
	SET_VARSIZE(ptr, len);

	set = (hashset_t *) ptr;

	set->flags = 0;
	set->maxelements = maxelements;
	set->nelements = 0;
	set->hashfn_id = JENKINS_LOOKUP3_HASHFN_ID;

	set->flags |= 0;

	return set;
}

Datum
hashset_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	char *endptr;
	int32 len = strlen(str);
	hashset_t *set;

	/* Check the opening and closing braces */
	if (str[0] != '{' || str[len - 1] != '}')
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for hashset: \"%s\"", str),
				errdetail("Hashset representation must start with \"{\" and end with \"}\".")));
	}

	/* Start parsing from the first number (after the opening brace) */
	str++;

	/* Initial size based on input length (arbitrary, could be optimized) */
	set = hashset_allocate(len/2);

	while (true)
	{
		int64 value = strtol(str, &endptr, 10);

		if (errno == ERANGE || value < PG_INT32_MIN || value > PG_INT32_MAX)
		{
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("value \"%s\" is out of range for type %s", str,
					"integer")));
		}

		/* Add the value to the hashset, resize if needed */
		if (set->nelements >= set->maxelements)
		{
			set = hashset_resize(set);
		}
		set = hashset_add_element(set, (int32)value);

		/* Error handling for strtol */
		if (endptr == str)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for integer: \"%s\"", str)));
		}
		else if (*endptr == ',')
		{
			str = endptr + 1;  /* Move to the next number */
		}
		else if (*endptr == '}')
		{
			break;  /* End of the hashset */
		}
		else
		{
			/* Unexpected character */
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("unexpected character \"%c\" in hashset input", *endptr)));
		}
	}

	PG_RETURN_POINTER(set);
}

Datum
hashset_out(PG_FUNCTION_ARGS)
{
	hashset_t *set = (hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *bitmap;
	int32 *values;
	int i;
	StringInfoData str;

	/* Calculate the pointer to the bitmap and values array */
	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->maxelements, 8));

	/* Initialize the StringInfo buffer */
	initStringInfo(&str);

	/* Append the opening brace for the output hashset string */
	appendStringInfoChar(&str, '{');

	/* Loop through the elements and append them to the string */
	for (i = 0; i < set->maxelements; i++)
	{
		int byte = i / 8;
		int bit = i % 8;

		/* Check if the bit in the bitmap is set */
		if (bitmap[byte] & (0x01 << bit))
		{
			/* Append the value */
			if (str.len > 1)
				appendStringInfoChar(&str, ',');
			appendStringInfo(&str, "%d", values[i]);
		}
	}

	/* Append the closing brace for the output hashset string */
	appendStringInfoChar(&str, '}');

	/* Return the resulting string */
	PG_RETURN_CSTRING(str.data);
}


Datum
hashset_send(PG_FUNCTION_ARGS)
{
	hashset_t  *set = (hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	StringInfoData buf;
	int32 data_size;

	/* Begin constructing the message */
	pq_begintypsend(&buf);

	/* Send the non-data fields */
	pq_sendint(&buf, set->flags, 4);
	pq_sendint(&buf, set->maxelements, 4);
	pq_sendint(&buf, set->nelements, 4);
	pq_sendint(&buf, set->hashfn_id, 4);

	/* Compute and send the size of the data field */
	data_size = VARSIZE(set) - offsetof(hashset_t, data);
	pq_sendbytes(&buf, set->data, data_size);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}


Datum
hashset_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	hashset_t	*set;
	int32		data_size;
	Size		total_size;
	const char	*binary_data;

	/* Read fields from buffer */
	int32 flags = pq_getmsgint(buf, 4);
	int32 maxelements = pq_getmsgint(buf, 4);
	int32 nelements = pq_getmsgint(buf, 4);
	int32 hashfn_id = pq_getmsgint(buf, 4);

	/* Compute the size of the data field */
	data_size = buf->len - buf->cursor;

	/* Read the binary data */
	binary_data = pq_getmsgbytes(buf, data_size);

	/* Compute total size of hashset_t */
	total_size = offsetof(hashset_t, data) + data_size;

	/* Allocate memory for hashset including the data field */
	set = (hashset_t *) palloc0(total_size);

	/* Set the size of the variable-length data structure */
	SET_VARSIZE(set, total_size);

	/* Populate the structure */
	set->flags = flags;
	set->maxelements = maxelements;
	set->nelements = nelements;
	set->hashfn_id = hashfn_id;
	memcpy(set->data, binary_data, data_size);

	PG_RETURN_POINTER(set);
}


Datum
hashset_to_array(PG_FUNCTION_ARGS)
{
	int				i,
					idx;
	hashset_t	   *set;
	int32		   *values;
	int				nvalues;

	char		   *sbitmap;
	int32		   *svalues;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = (hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	sbitmap = set->data;
	svalues = (int32 *) (set->data + CEIL_DIV(set->maxelements, 8));

	/* number of values to store in the array */
	nvalues = set->nelements;
	values = (int32 *) palloc(sizeof(int32) * nvalues);

	idx = 0;
	for (i = 0; i < set->maxelements; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (sbitmap[byte] & (0x01 << bit))
			values[idx++] = svalues[i];
	}

	Assert(idx == nvalues);

	return int32_to_array(fcinfo, values, nvalues);
}


/*
 * construct an SQL array from a simple C double array
 */
static Datum
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


static hashset_t *
hashset_resize(hashset_t * set)
{
	int			i;
	hashset_t	*new = hashset_allocate(set->maxelements * 2);
	char		*bitmap;
	int32		*values;

	/* Calculate the pointer to the bitmap and values array */
	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->maxelements, 8));

	for (i = 0; i < set->maxelements; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (bitmap[byte] & (0x01 << bit))
			hashset_add_element(new, values[i]);
	}

	return new;
}

static hashset_t *
hashset_add_element(hashset_t *set, int32 value)
{
	int		byte;
	int		bit;
	uint32	hash;
	char   *bitmap;
	int32  *values;

	if (set->nelements > set->maxelements * 0.75)
		set = hashset_resize(set);

	if (set->hashfn_id == JENKINS_LOOKUP3_HASHFN_ID)
	{
		hash = hash_bytes_uint32((uint32) value) % set->maxelements;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid hash function ID: \"%d\"", set->hashfn_id)));
	}

	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->maxelements, 8));

	while (true)
	{
		byte = (hash / 8);
		bit = (hash % 8);

		/* the item is already used - maybe it's the same value? */
		if (bitmap[byte] & (0x01 << bit))
		{
			/* same value, we're done */
			if (values[hash] == value)
				break;

			hash = (hash + HASHSET_STEP) % set->maxelements;
			continue;
		}

		/* found an empty spot, before hitting the value first */
		bitmap[byte] |= (0x01 << bit);
		values[hash] = value;

		set->nelements++;

		break;
	}

	return set;
}

static bool
hashset_contains_element(hashset_t *set, int32 value)
{
	int     byte;
	int     bit;
	uint32  hash;
	char   *bitmap;
	int32  *values;
	int     num_probes = 0; /* Add a counter for the number of probes */

	if (set->hashfn_id == JENKINS_LOOKUP3_HASHFN_ID)
	{
		hash = hash_bytes_uint32((uint32) value) % set->maxelements;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid hash function ID: \"%d\"", set->hashfn_id)));
	}

	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->maxelements, 8));

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
		hash = (hash + HASHSET_STEP) % set->maxelements;

		num_probes++; /* Increment the number of probes */

		/* Check if we have probed all slots */
		if (num_probes >= set->maxelements)
			return false; /* Avoid infinite loop */
	}
}

Datum
hashset_add(PG_FUNCTION_ARGS)
{
	hashset_t *set;

	if (PG_ARGISNULL(1))
	{
		if (PG_ARGISNULL(0))
			PG_RETURN_NULL();

		PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	/* if there's no hashset allocated, create it now */
	if (PG_ARGISNULL(0))
		set = hashset_allocate(64);
	else
	{
		/* make sure we are working with a non-toasted and non-shared copy of the input */
		set = (hashset_t *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	}

	set = hashset_add_element(set, PG_GETARG_INT32(1));

	PG_RETURN_POINTER(set);
}

Datum
hashset_merge(PG_FUNCTION_ARGS)
{
	int			i;

	hashset_t  *seta;
	hashset_t  *setb;

	char	   *bitmap;
	int32	   *values;

	/* */
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		PG_RETURN_NULL();
	else if (PG_ARGISNULL(1))
		PG_RETURN_POINTER(PG_GETARG_HASHSET(0));
	else if (PG_ARGISNULL(0))
		PG_RETURN_POINTER(PG_GETARG_HASHSET(1));

	seta = PG_GETARG_HASHSET(0);
	setb = PG_GETARG_HASHSET(1);

	bitmap = setb->data;
	values = (int32 *) (setb->data + CEIL_DIV(setb->maxelements, 8));

	for (i = 0; i < setb->maxelements; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (bitmap[byte] & (0x01 << bit))
			seta = hashset_add_element(seta, values[i]);
	}

	PG_RETURN_POINTER(seta);
}

Datum
hashset_init(PG_FUNCTION_ARGS)
{
	PG_RETURN_POINTER(hashset_allocate(PG_GETARG_INT32(0)));
}

Datum
hashset_contains(PG_FUNCTION_ARGS)
{
	hashset_t  *set;
	int32		value;

	if (PG_ARGISNULL(1) || PG_ARGISNULL(0))
		PG_RETURN_BOOL(false);

	set = PG_GETARG_HASHSET(0);
	value = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(hashset_contains_element(set, value));
}

Datum
hashset_count(PG_FUNCTION_ARGS)
{
	hashset_t  *set;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = PG_GETARG_HASHSET(0);

	PG_RETURN_INT64(set->nelements);
}

Datum
hashset_agg_add(PG_FUNCTION_ARGS)
{
	MemoryContext	oldcontext;
	hashset_t *state;

	MemoryContext aggcontext;

	/* cannot be called directly because of internal-type argument */
	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "hashset_add_add called in non-aggregate context");

	/*
	 * We want to skip NULL values altogether - we return either the existing
	 * hashset (if it already exists) or NULL.
	 */
	if (PG_ARGISNULL(1))
	{
		if (PG_ARGISNULL(0))
			PG_RETURN_NULL();

		/* if there already is a state accumulated, don't forget it */
		PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	/* if there's no hashset allocated, create it now */
	if (PG_ARGISNULL(0))
	{
		oldcontext = MemoryContextSwitchTo(aggcontext);
		state = hashset_allocate(64);
		MemoryContextSwitchTo(oldcontext);
	}
	else
		state = (hashset_t *) PG_GETARG_POINTER(0);

	oldcontext = MemoryContextSwitchTo(aggcontext);
	state = hashset_add_element(state, PG_GETARG_INT32(1));
	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}

Datum
hashset_agg_add_set(PG_FUNCTION_ARGS)
{
	MemoryContext	oldcontext;
	hashset_t *state;

	MemoryContext aggcontext;

	/* cannot be called directly because of internal-type argument */
	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "hashset_add_add called in non-aggregate context");

	/*
	 * We want to skip NULL values altogether - we return either the existing
	 * hashset (if it already exists) or NULL.
	 */
	if (PG_ARGISNULL(1))
	{
		if (PG_ARGISNULL(0))
			PG_RETURN_NULL();

		/* if there already is a state accumulated, don't forget it */
		PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	/* if there's no hashset allocated, create it now */
	if (PG_ARGISNULL(0))
	{
		oldcontext = MemoryContextSwitchTo(aggcontext);
		state = hashset_allocate(64);
		MemoryContextSwitchTo(oldcontext);
	}
	else
		state = (hashset_t *) PG_GETARG_POINTER(0);

	oldcontext = MemoryContextSwitchTo(aggcontext);

	{
		int			i;
		char	   *bitmap;
		int32	   *values;
		hashset_t  *value;

		value = PG_GETARG_HASHSET(1);

		bitmap = value->data;
		values = (int32 *) (value->data + CEIL_DIV(value->maxelements, 8));

		for (i = 0; i < value->maxelements; i++)
		{
			int	byte = (i / 8);
			int	bit = (i % 8);

			if (bitmap[byte] & (0x01 << bit))
				state = hashset_add_element(state, values[i]);
		}
	}

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}

Datum
hashset_agg_final(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}

static hashset_t *
hashset_copy(hashset_t *src)
{
	return src;
}

Datum
hashset_agg_combine(PG_FUNCTION_ARGS)
{
	int			i;
	hashset_t	 *src;
	hashset_t	 *dst;
	MemoryContext aggcontext;
	MemoryContext oldcontext;

	char		 *bitmap;
	int32		 *values;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "hashset_agg_combine called in non-aggregate context");

	/* if no "merged" state yet, try creating it */
	if (PG_ARGISNULL(0))
	{
		/* nope, the second argument is NULL to, so return NULL */
		if (PG_ARGISNULL(1))
			PG_RETURN_NULL();

		/* the second argument is not NULL, so copy it */
		src = (hashset_t *) PG_GETARG_POINTER(1);

		/* copy the hashset into the right long-lived memory context */
		oldcontext = MemoryContextSwitchTo(aggcontext);
		src = hashset_copy(src);
		MemoryContextSwitchTo(oldcontext);

		PG_RETURN_POINTER(src);
	}

	/*
	 * If the second argument is NULL, just return the first one (we know
	 * it's not NULL at this point).
	 */
	if (PG_ARGISNULL(1))
		PG_RETURN_DATUM(PG_GETARG_DATUM(0));

	/* Now we know neither argument is NULL, so merge them. */
	src = (hashset_t *) PG_GETARG_POINTER(1);
	dst = (hashset_t *) PG_GETARG_POINTER(0);

	bitmap = src->data;
	values = (int32 *) (src->data + CEIL_DIV(src->maxelements, 8));

	for (i = 0; i < src->maxelements; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (bitmap[byte] & (0x01 << bit))
			dst = hashset_add_element(dst, values[i]);
	}


	PG_RETURN_POINTER(dst);
}


Datum
hashset_equals(PG_FUNCTION_ARGS)
{
	hashset_t *a = PG_GETARG_HASHSET(0);
	hashset_t *b = PG_GETARG_HASHSET(1);

	char *bitmap_a;
	int32 *values_a;
	int i;

	/*
	 * Check if the number of elements is the same
	 */
	if (a->nelements != b->nelements)
		PG_RETURN_BOOL(false);

	bitmap_a = a->data;
	values_a = (int32 *)(a->data + CEIL_DIV(a->maxelements, 8));

	/*
	 * Check if every element in a is also in b
	 */
	for (i = 0; i < a->maxelements; i++)
	{
		int byte = (i / 8);
		int bit = (i % 8);

		if (bitmap_a[byte] & (0x01 << bit))
		{
			int32 value = values_a[i];

			if (!hashset_contains_element(b, value))
				PG_RETURN_BOOL(false);
		}
	}

	/*
	 * All elements in a are in b and the number of elements is the same,
	 * so the sets must be equal.
	 */
	PG_RETURN_BOOL(true);
}


Datum
hashset_neq(PG_FUNCTION_ARGS)
{
    hashset_t *a = PG_GETARG_HASHSET(0);
    hashset_t *b = PG_GETARG_HASHSET(1);

    /* If a is not equal to b, then they are not equal */
    if (!DatumGetBool(DirectFunctionCall2(hashset_equals, PointerGetDatum(a), PointerGetDatum(b))))
        PG_RETURN_BOOL(true);

    PG_RETURN_BOOL(false);
}


Datum hashset_hash(PG_FUNCTION_ARGS)
{
    hashset_t *set = PG_GETARG_HASHSET(0);

    /* Initial hash value */
    uint32 hash = 0;

    /* Access the data array */
    char *bitmap = set->data;
    int32 *values = (int32 *)(set->data + CEIL_DIV(set->maxelements, 8));

    /* Iterate through all elements */
    for (int32 i = 0; i < set->maxelements; i++)
    {
        int byte = i / 8;
        int bit = i % 8;

        /* Check if the current position is occupied */
        if (bitmap[byte] & (0x01 << bit))
        {
            /* Combine the hash value of the current element with the total hash */
            hash = hash_combine(hash, hash_uint32(values[i]));
        }
    }

    /* Return the final hash value */
    PG_RETURN_INT32(hash);
}


Datum
hashset_lt(PG_FUNCTION_ARGS)
{
    hashset_t *a = PG_GETARG_HASHSET(0);
    hashset_t *b = PG_GETARG_HASHSET(1);
    int32 cmp;

    cmp = DatumGetInt32(DirectFunctionCall2(hashset_cmp,
                                            PointerGetDatum(a),
                                            PointerGetDatum(b)));

    PG_RETURN_BOOL(cmp < 0);
}


Datum
hashset_le(PG_FUNCTION_ARGS)
{
	hashset_t *a = PG_GETARG_HASHSET(0);
	hashset_t *b = PG_GETARG_HASHSET(1);
	int32 cmp;

	cmp = DatumGetInt32(DirectFunctionCall2(hashset_cmp,
											PointerGetDatum(a),
											PointerGetDatum(b)));

	PG_RETURN_BOOL(cmp <= 0);
}


Datum
hashset_gt(PG_FUNCTION_ARGS)
{
	hashset_t *a = PG_GETARG_HASHSET(0);
	hashset_t *b = PG_GETARG_HASHSET(1);
	int32 cmp;

	cmp = DatumGetInt32(DirectFunctionCall2(hashset_cmp,
											PointerGetDatum(a),
											PointerGetDatum(b)));

	PG_RETURN_BOOL(cmp > 0);
}


Datum
hashset_ge(PG_FUNCTION_ARGS)
{
	hashset_t *a = PG_GETARG_HASHSET(0);
	hashset_t *b = PG_GETARG_HASHSET(1);
	int32 cmp;

	cmp = DatumGetInt32(DirectFunctionCall2(hashset_cmp,
											PointerGetDatum(a),
											PointerGetDatum(b)));

	PG_RETURN_BOOL(cmp >= 0);
}


Datum
hashset_cmp(PG_FUNCTION_ARGS)
{
	hashset_t *a = PG_GETARG_HASHSET(0);
	hashset_t *b = PG_GETARG_HASHSET(1);

	char *bitmap_a, *bitmap_b;
	int32 *values_a, *values_b;
	int i = 0, j = 0;

	bitmap_a = a->data;
	values_a = (int32 *)(a->data + CEIL_DIV(a->maxelements, 8));

	bitmap_b = b->data;
	values_b = (int32 *)(b->data + CEIL_DIV(b->maxelements, 8));

	/* Iterate over the elements in each hashset independently */
	while(i < a->maxelements && j < b->maxelements)
	{
		int byte_a = (i / 8);
		int bit_a = (i % 8);

		int byte_b = (j / 8);
		int bit_b = (j % 8);

		bool has_elem_a = bitmap_a[byte_a] & (0x01 << bit_a);
		bool has_elem_b = bitmap_b[byte_b] & (0x01 << bit_b);

		int32 value_a;
		int32 value_b;

		/* Skip if position is empty in either bitmap */
		if (!has_elem_a)
		{
			i++;
			continue;
		}

		if (!has_elem_b)
		{
			j++;
			continue;
		}

		/* Both hashsets have an element at the current position */
		value_a = values_a[i++];
		value_b = values_b[j++];

		if (value_a < value_b)
			PG_RETURN_INT32(-1);
		else if (value_a > value_b)
			PG_RETURN_INT32(1);
	}

	/*
	 * If all compared elements are equal,
	 * then compare the remaining elements in the larger hashset
	 */
	if (i < a->maxelements)
		PG_RETURN_INT32(1);
	else if (j < b->maxelements)
		PG_RETURN_INT32(-1);
	else
		PG_RETURN_INT32(0);
}
