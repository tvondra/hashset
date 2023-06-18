#include "hashset.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>

#define PG_GETARG_INT4HASHSET(x)        (int4hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(x))
#define PG_GETARG_INT4HASHSET_COPY(x)   (int4hashset_t *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(x))

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(int4hashset_in);
PG_FUNCTION_INFO_V1(int4hashset_out);
PG_FUNCTION_INFO_V1(int4hashset_send);
PG_FUNCTION_INFO_V1(int4hashset_recv);
PG_FUNCTION_INFO_V1(int4hashset_add);
PG_FUNCTION_INFO_V1(int4hashset_contains);
PG_FUNCTION_INFO_V1(int4hashset_count);
PG_FUNCTION_INFO_V1(int4hashset_merge);
PG_FUNCTION_INFO_V1(int4hashset_init);
PG_FUNCTION_INFO_V1(int4hashset_capacity);
PG_FUNCTION_INFO_V1(int4hashset_collisions);
PG_FUNCTION_INFO_V1(int4hashset_max_collisions);
PG_FUNCTION_INFO_V1(int4hashset_agg_add);
PG_FUNCTION_INFO_V1(int4hashset_agg_add_set);
PG_FUNCTION_INFO_V1(int4hashset_agg_final);
PG_FUNCTION_INFO_V1(int4hashset_agg_combine);
PG_FUNCTION_INFO_V1(int4hashset_to_array);
PG_FUNCTION_INFO_V1(int4hashset_equals);
PG_FUNCTION_INFO_V1(int4hashset_neq);
PG_FUNCTION_INFO_V1(int4hashset_hash);
PG_FUNCTION_INFO_V1(int4hashset_lt);
PG_FUNCTION_INFO_V1(int4hashset_le);
PG_FUNCTION_INFO_V1(int4hashset_gt);
PG_FUNCTION_INFO_V1(int4hashset_ge);
PG_FUNCTION_INFO_V1(int4hashset_cmp);

Datum int4hashset_in(PG_FUNCTION_ARGS);
Datum int4hashset_out(PG_FUNCTION_ARGS);
Datum int4hashset_send(PG_FUNCTION_ARGS);
Datum int4hashset_recv(PG_FUNCTION_ARGS);
Datum int4hashset_add(PG_FUNCTION_ARGS);
Datum int4hashset_contains(PG_FUNCTION_ARGS);
Datum int4hashset_count(PG_FUNCTION_ARGS);
Datum int4hashset_merge(PG_FUNCTION_ARGS);
Datum int4hashset_init(PG_FUNCTION_ARGS);
Datum int4hashset_capacity(PG_FUNCTION_ARGS);
Datum int4hashset_collisions(PG_FUNCTION_ARGS);
Datum int4hashset_max_collisions(PG_FUNCTION_ARGS);
Datum int4hashset_agg_add(PG_FUNCTION_ARGS);
Datum int4hashset_agg_add_set(PG_FUNCTION_ARGS);
Datum int4hashset_agg_final(PG_FUNCTION_ARGS);
Datum int4hashset_agg_combine(PG_FUNCTION_ARGS);
Datum int4hashset_to_array(PG_FUNCTION_ARGS);
Datum int4hashset_equals(PG_FUNCTION_ARGS);
Datum int4hashset_neq(PG_FUNCTION_ARGS);
Datum int4hashset_hash(PG_FUNCTION_ARGS);
Datum int4hashset_lt(PG_FUNCTION_ARGS);
Datum int4hashset_le(PG_FUNCTION_ARGS);
Datum int4hashset_gt(PG_FUNCTION_ARGS);
Datum int4hashset_ge(PG_FUNCTION_ARGS);
Datum int4hashset_cmp(PG_FUNCTION_ARGS);

Datum
int4hashset_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	char *endptr;
	int32 len = strlen(str);
	int4hashset_t *set;
	int64 value;

	/* Skip initial spaces */
	while (hashset_isspace(*str)) str++;

	/* Check the opening brace */
	if (*str != '{')
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for hashset: \"%s\"", str),
				errdetail("Hashset representation must start with \"{\".")));
	}

	/* Start parsing from the first number (after the opening brace) */
	str++;

	/* Initial size based on input length (arbitrary, could be optimized) */
	set = int4hashset_allocate(
		len/2,
		DEFAULT_LOAD_FACTOR,
		DEFAULT_GROWTH_FACTOR,
		DEFAULT_HASHFN_ID
	);

	while (true)
	{
		/* Skip spaces before number */
		while (hashset_isspace(*str)) str++;

		/* Check for closing brace, handling the case for an empty set */
		if (*str == '}')
		{
			str++; /* Move past the closing brace */
			break;
		}

		/* Parse the number */
		value = strtol(str, &endptr, 10);

		if (errno == ERANGE || value < PG_INT32_MIN || value > PG_INT32_MAX)
		{
			ereport(ERROR,
					(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("value \"%s\" is out of range for type %s", str,
							"integer")));
		}

		/* Add the value to the hashset, resize if needed */
		if (set->nelements >= set->capacity)
		{
			set = int4hashset_resize(set);
		}
		set = int4hashset_add_element(set, (int32)value);

		/* Error handling for strtol */
		if (endptr == str)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for integer: \"%s\"", str)));
		}

		str = endptr; /* Move to next potential number or closing brace */

        /* Skip spaces before the next number or closing brace */
		while (hashset_isspace(*str)) str++;

		if (*str == ',')
		{
			str++; /* Skip comma before next loop iteration */
		}
		else if (*str != '}')
		{
			/* Unexpected character */
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("unexpected character \"%c\" in hashset input", *str)));
		}
	}

	/* Only whitespace is allowed after the closing brace */
	while (*str)
	{
		if (!hashset_isspace(*str))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("malformed hashset literal: \"%s\"", str),
					errdetail("Junk after closing right brace.")));
		}
		str++;
	}

	PG_RETURN_POINTER(set);
}

Datum
int4hashset_out(PG_FUNCTION_ARGS)
{
	int4hashset_t *set = PG_GETARG_INT4HASHSET(0);
	char *bitmap;
	int32 *values;
	int i;
	StringInfoData str;

	/* Calculate the pointer to the bitmap and values array */
	bitmap = set->data;
	values = (int32 *) (set->data + CEIL_DIV(set->capacity, 8));

	/* Initialize the StringInfo buffer */
	initStringInfo(&str);

	/* Append the opening brace for the output hashset string */
	appendStringInfoChar(&str, '{');

	/* Loop through the elements and append them to the string */
	for (i = 0; i < set->capacity; i++)
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
int4hashset_send(PG_FUNCTION_ARGS)
{
	int4hashset_t  *set = PG_GETARG_INT4HASHSET(0);
	StringInfoData	buf;
	int32			data_size;
	int				version = 1;

	/* Begin constructing the message */
	pq_begintypsend(&buf);

	/* Send the version number */
	pq_sendint8(&buf, version);

	/* Send the non-data fields */
	pq_sendint32(&buf, set->flags);
	pq_sendint32(&buf, set->capacity);
	pq_sendint32(&buf, set->nelements);
	pq_sendint32(&buf, set->hashfn_id);
	pq_sendfloat4(&buf, set->load_factor);
	pq_sendfloat4(&buf, set->growth_factor);
	pq_sendint32(&buf, set->ncollisions);
	pq_sendint32(&buf, set->max_collisions);
	pq_sendint32(&buf, set->hash);

	/* Compute and send the size of the data field */
	data_size = VARSIZE(set) - offsetof(int4hashset_t, data);
	pq_sendbytes(&buf, set->data, data_size);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum
int4hashset_recv(PG_FUNCTION_ARGS)
{
	StringInfo		buf = (StringInfo) PG_GETARG_POINTER(0);
	int4hashset_t  *set;
	int32			data_size;
	Size			total_size;
	const char	   *binary_data;
	int				version;
	int32			flags;
	int32			capacity;
	int32			nelements;
	int32			hashfn_id;
	float4			load_factor;
	float4			growth_factor;
	int32			ncollisions;
	int32			max_collisions;
	int32			hash;

	version = pq_getmsgint(buf, 1);
	if (version != 1)
		elog(ERROR, "unsupported hashset version number %d", version);

	/* Read fields from buffer */
	flags = pq_getmsgint(buf, 4);
	capacity = pq_getmsgint(buf, 4);
	nelements = pq_getmsgint(buf, 4);
	hashfn_id = pq_getmsgint(buf, 4);
	load_factor = pq_getmsgfloat4(buf);
	growth_factor = pq_getmsgfloat4(buf);
	ncollisions = pq_getmsgint(buf, 4);
	max_collisions = pq_getmsgint(buf, 4);
	hash = pq_getmsgint(buf, 4);

	/* Compute the size of the data field */
	data_size = buf->len - buf->cursor;

	/* Read the binary data */
	binary_data = pq_getmsgbytes(buf, data_size);

	/* Make sure that there is no extra data left in the message */
	pq_getmsgend(buf);

	/* Compute total size of hashset_t */
	total_size = offsetof(int4hashset_t, data) + data_size;

	/* Allocate memory for hashset including the data field */
	set = (int4hashset_t *) palloc0(total_size);

	/* Set the size of the variable-length data structure */
	SET_VARSIZE(set, total_size);

	/* Populate the structure */
	set->flags = flags;
	set->capacity = capacity;
	set->nelements = nelements;
	set->hashfn_id = hashfn_id;
	set->load_factor = load_factor;
	set->growth_factor = growth_factor;
	set->ncollisions = ncollisions;
	set->max_collisions = max_collisions;
	set->hash = hash;
	memcpy(set->data, binary_data, data_size);

	PG_RETURN_POINTER(set);
}

Datum
int4hashset_add(PG_FUNCTION_ARGS)
{
	int4hashset_t *set;

	if (PG_ARGISNULL(1))
	{
		if (PG_ARGISNULL(0))
			PG_RETURN_NULL();

		PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	/* if there's no hashset allocated, create it now */
	if (PG_ARGISNULL(0))
	{
		set = int4hashset_allocate(
			DEFAULT_INITIAL_CAPACITY,
			DEFAULT_LOAD_FACTOR,
			DEFAULT_GROWTH_FACTOR,
			DEFAULT_HASHFN_ID
		);
	}
	else
	{
		/* make sure we are working with a non-toasted and non-shared copy of the input */
		set = PG_GETARG_INT4HASHSET_COPY(0);
	}

	set = int4hashset_add_element(set, PG_GETARG_INT32(1));

	PG_RETURN_POINTER(set);
}

Datum
int4hashset_contains(PG_FUNCTION_ARGS)
{
	int4hashset_t  *set;
	int32			value;

	if (PG_ARGISNULL(1) || PG_ARGISNULL(0))
		PG_RETURN_BOOL(false);

	set = PG_GETARG_INT4HASHSET(0);
	value = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(int4hashset_contains_element(set, value));
}

Datum
int4hashset_count(PG_FUNCTION_ARGS)
{
	int4hashset_t	*set;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = PG_GETARG_INT4HASHSET(0);

	PG_RETURN_INT64(set->nelements);
}

Datum
int4hashset_merge(PG_FUNCTION_ARGS)
{
	int				i;

	int4hashset_t  *seta;
	int4hashset_t  *setb;

	char		   *bitmap;
	int32_t		   *values;

	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		PG_RETURN_NULL();
	else if (PG_ARGISNULL(1))
		PG_RETURN_POINTER(PG_GETARG_INT4HASHSET(0));
	else if (PG_ARGISNULL(0))
		PG_RETURN_POINTER(PG_GETARG_INT4HASHSET(1));

	seta = PG_GETARG_INT4HASHSET_COPY(0);
	setb = PG_GETARG_INT4HASHSET(1);

	bitmap = setb->data;
	values = (int32 *) (setb->data + CEIL_DIV(setb->capacity, 8));

	for (i = 0; i < setb->capacity; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (bitmap[byte] & (0x01 << bit))
			seta = int4hashset_add_element(seta, values[i]);
	}

	PG_RETURN_POINTER(seta);
}

Datum
int4hashset_init(PG_FUNCTION_ARGS)
{
	int4hashset_t *set;
	int32 initial_capacity = PG_GETARG_INT32(0);
	float4 load_factor = PG_GETARG_FLOAT4(1);
	float4 growth_factor = PG_GETARG_FLOAT4(2);
	int32 hashfn_id = PG_GETARG_INT32(3);

	/* Validate input arguments */
	if (!(initial_capacity >= 0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("initial capacity cannot be negative")));
	}

	if (!(load_factor > 0.0 && load_factor < 1.0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("load factor must be between 0.0 and 1.0")));
	}

	if (!(growth_factor > 1.0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("growth factor must be greater than 1.0")));
	}

	if (!(hashfn_id == JENKINS_LOOKUP3_HASHFN_ID ||
	      hashfn_id == MURMURHASH32_HASHFN_ID ||
		  hashfn_id == NAIVE_HASHFN_ID))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Invalid hash function ID")));
	}

	set = int4hashset_allocate(
		initial_capacity,
		load_factor,
		growth_factor,
		hashfn_id
	);

	PG_RETURN_POINTER(set);
}

Datum
int4hashset_capacity(PG_FUNCTION_ARGS)
{
	int4hashset_t	*set;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = PG_GETARG_INT4HASHSET(0);

	PG_RETURN_INT64(set->capacity);
}

Datum
int4hashset_collisions(PG_FUNCTION_ARGS)
{
	int4hashset_t	*set;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = PG_GETARG_INT4HASHSET(0);

	PG_RETURN_INT64(set->ncollisions);
}

Datum
int4hashset_max_collisions(PG_FUNCTION_ARGS)
{
	int4hashset_t	*set;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = PG_GETARG_INT4HASHSET(0);

	PG_RETURN_INT64(set->max_collisions);
}

Datum
int4hashset_agg_add(PG_FUNCTION_ARGS)
{
	MemoryContext	oldcontext;
	int4hashset_t  *state;

	MemoryContext	aggcontext;

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
		state = int4hashset_allocate(
			DEFAULT_INITIAL_CAPACITY,
			DEFAULT_LOAD_FACTOR,
			DEFAULT_GROWTH_FACTOR,
			DEFAULT_HASHFN_ID
		);
		MemoryContextSwitchTo(oldcontext);
	}
	else
		state = (int4hashset_t *) PG_GETARG_POINTER(0);

	oldcontext = MemoryContextSwitchTo(aggcontext);
	state = int4hashset_add_element(state, PG_GETARG_INT32(1));
	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}

Datum
int4hashset_agg_add_set(PG_FUNCTION_ARGS)
{
	MemoryContext	oldcontext;
	int4hashset_t  *state;

	MemoryContext   aggcontext;

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
		state = int4hashset_allocate(
			DEFAULT_INITIAL_CAPACITY,
			DEFAULT_LOAD_FACTOR,
			DEFAULT_GROWTH_FACTOR,
			DEFAULT_HASHFN_ID
		);
		MemoryContextSwitchTo(oldcontext);
	}
	else
		state = (int4hashset_t *) PG_GETARG_POINTER(0);

	oldcontext = MemoryContextSwitchTo(aggcontext);

	{
		int				i;
		char		   *bitmap;
		int32		   *values;
		int4hashset_t  *value;

		value = PG_GETARG_INT4HASHSET(1);

		bitmap = value->data;
		values = (int32 *) (value->data + CEIL_DIV(value->capacity, 8));

		for (i = 0; i < value->capacity; i++)
		{
			int	byte = (i / 8);
			int	bit = (i % 8);

			if (bitmap[byte] & (0x01 << bit))
				state = int4hashset_add_element(state, values[i]);
		}
	}

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}

Datum
int4hashset_agg_final(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}

Datum
int4hashset_agg_combine(PG_FUNCTION_ARGS)
{
	int				i;
	int4hashset_t  *src;
	int4hashset_t  *dst;
	MemoryContext	aggcontext;
	MemoryContext	oldcontext;

	char		   *bitmap;
	int32		   *values;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "hashset_agg_combine called in non-aggregate context");

	/* if no "merged" state yet, try creating it */
	if (PG_ARGISNULL(0))
	{
		/* nope, the second argument is NULL to, so return NULL */
		if (PG_ARGISNULL(1))
			PG_RETURN_NULL();

		/* the second argument is not NULL, so copy it */
		src = (int4hashset_t *) PG_GETARG_POINTER(1);

		/* copy the hashset into the right long-lived memory context */
		oldcontext = MemoryContextSwitchTo(aggcontext);
		src = int4hashset_copy(src);
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
	src = (int4hashset_t *) PG_GETARG_POINTER(1);
	dst = (int4hashset_t *) PG_GETARG_POINTER(0);

	bitmap = src->data;
	values = (int32 *) (src->data + CEIL_DIV(src->capacity, 8));

	for (i = 0; i < src->capacity; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (bitmap[byte] & (0x01 << bit))
			dst = int4hashset_add_element(dst, values[i]);
	}


	PG_RETURN_POINTER(dst);
}

Datum
int4hashset_to_array(PG_FUNCTION_ARGS)
{
	int					i,
						idx;
	int4hashset_t	   *set;
	int32			   *values;
	int					nvalues;

	char			   *sbitmap;
	int32			   *svalues;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = PG_GETARG_INT4HASHSET(0);

	sbitmap = set->data;
	svalues = (int32 *) (set->data + CEIL_DIV(set->capacity, 8));

	/* number of values to store in the array */
	nvalues = set->nelements;
	values = (int32 *) palloc(sizeof(int32) * nvalues);

	idx = 0;
	for (i = 0; i < set->capacity; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (sbitmap[byte] & (0x01 << bit))
			values[idx++] = svalues[i];
	}

	Assert(idx == nvalues);

	return int32_to_array(fcinfo, values, nvalues);
}

Datum
int4hashset_equals(PG_FUNCTION_ARGS)
{
	int4hashset_t *a = PG_GETARG_INT4HASHSET(0);
	int4hashset_t *b = PG_GETARG_INT4HASHSET(1);

	char *bitmap_a;
	int32 *values_a;
	int i;

	/*
	 * Check if the number of elements is the same
	 */
	if (a->nelements != b->nelements)
		PG_RETURN_BOOL(false);

	bitmap_a = a->data;
	values_a = (int32 *)(a->data + CEIL_DIV(a->capacity, 8));

	/*
	 * Check if every element in a is also in b
	 */
	for (i = 0; i < a->capacity; i++)
	{
		int byte = (i / 8);
		int bit = (i % 8);

		if (bitmap_a[byte] & (0x01 << bit))
		{
			int32 value = values_a[i];

			if (!int4hashset_contains_element(b, value))
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
int4hashset_neq(PG_FUNCTION_ARGS)
{
    int4hashset_t *a = PG_GETARG_INT4HASHSET(0);
    int4hashset_t *b = PG_GETARG_INT4HASHSET(1);

    /* If a is not equal to b, then they are not equal */
    if (!DatumGetBool(DirectFunctionCall2(int4hashset_equals, PointerGetDatum(a), PointerGetDatum(b))))
        PG_RETURN_BOOL(true);

    PG_RETURN_BOOL(false);
}


Datum int4hashset_hash(PG_FUNCTION_ARGS)
{
    int4hashset_t *set = PG_GETARG_INT4HASHSET(0);

    PG_RETURN_INT32(set->hash);
}


Datum
int4hashset_lt(PG_FUNCTION_ARGS)
{
    int4hashset_t *a = PG_GETARG_INT4HASHSET(0);
    int4hashset_t *b = PG_GETARG_INT4HASHSET(1);
    int32 cmp;

    cmp = DatumGetInt32(DirectFunctionCall2(int4hashset_cmp,
                                            PointerGetDatum(a),
                                            PointerGetDatum(b)));

    PG_RETURN_BOOL(cmp < 0);
}


Datum
int4hashset_le(PG_FUNCTION_ARGS)
{
	int4hashset_t *a = PG_GETARG_INT4HASHSET(0);
	int4hashset_t *b = PG_GETARG_INT4HASHSET(1);
	int32 cmp;

	cmp = DatumGetInt32(DirectFunctionCall2(int4hashset_cmp,
											PointerGetDatum(a),
											PointerGetDatum(b)));

	PG_RETURN_BOOL(cmp <= 0);
}


Datum
int4hashset_gt(PG_FUNCTION_ARGS)
{
	int4hashset_t *a = PG_GETARG_INT4HASHSET(0);
	int4hashset_t *b = PG_GETARG_INT4HASHSET(1);
	int32 cmp;

	cmp = DatumGetInt32(DirectFunctionCall2(int4hashset_cmp,
											PointerGetDatum(a),
											PointerGetDatum(b)));

	PG_RETURN_BOOL(cmp > 0);
}


Datum
int4hashset_ge(PG_FUNCTION_ARGS)
{
	int4hashset_t *a = PG_GETARG_INT4HASHSET(0);
	int4hashset_t *b = PG_GETARG_INT4HASHSET(1);
	int32 cmp;

	cmp = DatumGetInt32(DirectFunctionCall2(int4hashset_cmp,
											PointerGetDatum(a),
											PointerGetDatum(b)));

	PG_RETURN_BOOL(cmp >= 0);
}

Datum
int4hashset_cmp(PG_FUNCTION_ARGS)
{
	int4hashset_t *a = PG_GETARG_INT4HASHSET(0);
	int4hashset_t *b = PG_GETARG_INT4HASHSET(1);
	int32		  *elements_a;
	int32		  *elements_b;

	/*
	 * Compare the hashes first, if they are different,
	 * we can immediately tell which set is 'greater'
	 */
	if (a->hash < b->hash)
		PG_RETURN_INT32(-1);
	else if (a->hash > b->hash)
		PG_RETURN_INT32(1);

	/*
	 * If hashes are equal, perform a more rigorous comparison
	 */

	/*
	 * If number of elements are different,
	 * we can use that to deterministically return -1 or 1
	 */
	if (a->nelements < b->nelements)
		PG_RETURN_INT32(-1);
	else if (a->nelements > b->nelements)
		PG_RETURN_INT32(1);

	/* Assert that the number of elements in both hashsets are equal */
	Assert(a->nelements == b->nelements);

	/* Extract and sort elements from each set */
	elements_a = int4hashset_extract_sorted_elements(a);
	elements_b = int4hashset_extract_sorted_elements(b);

	/* Now we can perform a lexicographical comparison */
	for (int32 i = 0; i < a->nelements; i++)
	{
		if (elements_a[i] < elements_b[i])
		{
			pfree(elements_a);
			pfree(elements_b);
			PG_RETURN_INT32(-1);
		}
		else if (elements_a[i] > elements_b[i])
		{
			pfree(elements_a);
			pfree(elements_b);
			PG_RETURN_INT32(1);
		}
	}

	/* All elements are equal, so the sets are equal */
	pfree(elements_a);
	pfree(elements_b);
	PG_RETURN_INT32(0);
}
