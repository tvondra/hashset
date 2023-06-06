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

PG_MODULE_MAGIC;

/*
 * hashset
 */
typedef struct hashset_t {
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int32		flags;			/* reserved for future use (versioning, ...) */
	int32		maxelements;	/* max number of element we have space for */
	int32		nelements;			/* number of items added to the hashset */
	char		data[FLEXIBLE_ARRAY_MEMBER];
} hashset_t;

static hashset_t *hashset_resize(hashset_t * set);
static hashset_t *hashset_add_element(hashset_t *set, int32 value);
static bool hashset_contains_element(hashset_t *set, int32 value);

static Datum int32_to_array(FunctionCallInfo fcinfo, int32 * d, int len);

#define PG_GETARG_HASHSET(x)	(hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(x))

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

/* allocate hashset with enough space for a requested number of centroids */
static hashset_t *
hashset_allocate(int maxelements)
{
	Size		len;
	hashset_t  *set;
	char	   *ptr;

	len = offsetof(hashset_t, data);
	len += (maxelements + 7) / 8;
	len += maxelements * sizeof(int32);

	/* we pre-allocate the array for all centroids and also the buffer for incoming data */
	ptr = palloc0(len);
	SET_VARSIZE(ptr, len);

	set = (hashset_t *) ptr;

	set->flags = 0;
	set->maxelements = maxelements;
	set->nelements = 0;

	/* new tdigest are automatically storing mean */
	set->flags |= 0;

	return set;
}

Datum
hashset_in(PG_FUNCTION_ARGS)
{
//	int			i, r;
//	char	   *str = PG_GETARG_CSTRING(0);
//	hashset_t  *set = NULL;

	PG_RETURN_NULL();
}

Datum
hashset_out(PG_FUNCTION_ARGS)
{
	//int			i;
	//tdigest_t  *digest = (tdigest_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	StringInfoData	str;

	initStringInfo(&str);

	PG_RETURN_CSTRING(str.data);
}

Datum
hashset_recv(PG_FUNCTION_ARGS)
{
	//StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	hashset_t  *set= NULL;

	PG_RETURN_POINTER(set);
}

Datum
hashset_send(PG_FUNCTION_ARGS)
{
	hashset_t  *set = (hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	StringInfoData buf;

	pq_begintypsend(&buf);

	pq_sendint(&buf, set->flags, 4);
	pq_sendint64(&buf, set->maxelements);
	pq_sendint(&buf, set->nelements, 4);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}


/*
 * tdigest_to_array
 *		Transform the tdigest into an array of double values.
 *
 * The whole digest is stored in a single "double precision" array, which
 * may be a bit confusing and perhaps fragile if more fields need to be
 * added in the future. The initial elements are flags, count (number of
 * items added to the digest), compression (determines the limit on number
 * of centroids) and current number of centroids. Follows stream of values
 * encoding the centroids in pairs of (mean, count).
 *
 * We make sure to always print mean, even for tdigests in the older format
 * storing sum for centroids. Otherwise the "mean" key would be confusing.
 * But we don't call tdigest_update_format, and instead we simply update the
 * flags and convert the sum/mean values.
 */
Datum
hashset_to_array(PG_FUNCTION_ARGS)
{
	int				i,
					idx;
	hashset_t	   *set;
	int32		   *values;
	int				nvalues;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	set = (hashset_t *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* number of values to store in the array */
	nvalues = set->nelements;
	values = (int32 *) palloc(sizeof(int32) * nvalues);

	idx = 0;
	for (i = 0; i < set->nelements; i++)
	{
		values[idx++] = i;
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
	int				i;
	hashset_t	   *new = hashset_allocate(set->maxelements * 2);
	char		   *bitmap;
	int32		   *values;

	bitmap = set->data;
	values = (int32 *) (set->data + set->maxelements / 8);

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

	hash = ((uint32) value * 7691 + 4201) % set->maxelements;

	bitmap = set->data;
	values = (int32 *) (set->data + set->maxelements / 8);

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

			hash = (hash + 13) % set->maxelements;
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
	int		byte;
	int		bit;
	uint32	hash;
	char   *bitmap;
	int32  *values;

	hash = ((uint32) value * 7691 + 4201) % set->maxelements;

	bitmap = set->data;
	values = (int32 *) (set->data + set->maxelements / 8);

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
		hash = (hash + 13) % set->maxelements;
	}

	return set;
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
		set = (hashset_t *) PG_GETARG_POINTER(0);

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
	values = (int32 *) (setb->data + setb->maxelements / 8);

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
	 * t-digest (if it already exists) or NULL.
	 */
	if (PG_ARGISNULL(1))
	{
		if (PG_ARGISNULL(0))
			PG_RETURN_NULL();

		/* if there already is a state accumulated, don't forget it */
		PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	/* if there's no digest allocated, create it now */
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
	 * t-digest (if it already exists) or NULL.
	 */
	if (PG_ARGISNULL(1))
	{
		if (PG_ARGISNULL(0))
			PG_RETURN_NULL();

		/* if there already is a state accumulated, don't forget it */
		PG_RETURN_DATUM(PG_GETARG_DATUM(0));
	}

	/* if there's no digest allocated, create it now */
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
		values = (int32 *) (value->data + value->maxelements / 8);

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
		elog(ERROR, "tdigest_combine called in non-aggregate context");

	/* if no "merged" state yet, try creating it */
	if (PG_ARGISNULL(0))
	{
		/* nope, the second argument is NULL to, so return NULL */
		if (PG_ARGISNULL(1))
			PG_RETURN_NULL();

		/* the second argument is not NULL, so copy it */
		src = (hashset_t *) PG_GETARG_POINTER(1);

		/* copy the digest into the right long-lived memory context */
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
	values = (int32 *) (src->data + src->maxelements / 8);

	for (i = 0; i < src->maxelements; i++)
	{
		int	byte = (i / 8);
		int	bit = (i % 8);

		if (bitmap[byte] & (0x01 << bit))
			dst = hashset_add_element(dst, values[i]);
	}


	PG_RETURN_POINTER(dst);
}
