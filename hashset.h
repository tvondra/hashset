#ifndef HASHSET_H
#define HASHSET_H

#include "postgres.h"
#include "libpq/pqformat.h"
#include "nodes/memnodes.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "catalog/pg_type.h"
#include "common/hashfn.h"

#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define HASHSET_STEP 13
#define JENKINS_LOOKUP3_HASHFN_ID 1
#define MURMURHASH32_HASHFN_ID 2
#define NAIVE_HASHFN_ID 3
#define NAIVE_HASHFN_MULTIPLIER 7691
#define NAIVE_HASHFN_INCREMENT 4201

/*
 * These defaults should match the the SQL function int4hashset()
 */
#define DEFAULT_INITIAL_CAPACITY 0
#define DEFAULT_LOAD_FACTOR 0.75
#define DEFAULT_GROWTH_FACTOR 2.0
#define DEFAULT_HASHFN_ID JENKINS_LOOKUP3_HASHFN_ID

typedef struct int4hashset_t {
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int32		flags;			/* reserved for future use (versioning, ...) */
	int32		capacity;		/* max number of element we have space for */
	int32		nelements;		/* number of items added to the hashset */
	int32		hashfn_id;		/* ID of the hash function used */
	float4		load_factor;	/* Load factor before triggering resize */
	float4		growth_factor;	/* Growth factor when resizing the hashset */
	int32		ncollisions;	/* Number of collisions */
	int32		max_collisions;	/* Maximum collisions for a single element */
	int32		hash;			/* Stored hash value of the hashset */
	char		data[FLEXIBLE_ARRAY_MEMBER];
} int4hashset_t;

int4hashset_t *int4hashset_allocate(int capacity, float4 load_factor, float4 growth_factor, int hashfn_id);
int4hashset_t *int4hashset_resize(int4hashset_t * set);
int4hashset_t *int4hashset_add_element(int4hashset_t *set, int32 value);
bool int4hashset_contains_element(int4hashset_t *set, int32 value);
int32 *int4hashset_extract_sorted_elements(int4hashset_t *set);
int4hashset_t *int4hashset_copy(int4hashset_t *src);
bool hashset_isspace(char ch);
Datum int32_to_array(FunctionCallInfo fcinfo, int32 *d, int len);

#endif /* HASHSET_H */
