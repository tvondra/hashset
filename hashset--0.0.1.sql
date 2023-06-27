/*
 * Hashset Type Definition
 */

CREATE TYPE int4hashset;

CREATE OR REPLACE FUNCTION int4hashset_in(cstring)
RETURNS int4hashset
AS 'hashset', 'int4hashset_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION int4hashset_out(int4hashset)
RETURNS cstring
AS 'hashset', 'int4hashset_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION int4hashset_send(int4hashset)
RETURNS bytea
AS 'hashset', 'int4hashset_send'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION int4hashset_recv(internal)
RETURNS int4hashset
AS 'hashset', 'int4hashset_recv'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE int4hashset (
    INPUT = int4hashset_in,
    OUTPUT = int4hashset_out,
    RECEIVE = int4hashset_recv,
    SEND = int4hashset_send,
    INTERNALLENGTH = variable,
    STORAGE = external
);

/*
 * Hashset Functions
 */

CREATE OR REPLACE FUNCTION int4hashset(
    capacity int DEFAULT 0,
    load_factor float4 DEFAULT 0.75,
    growth_factor float4 DEFAULT 2.0,
    hashfn_id int DEFAULT 1
)
RETURNS int4hashset
AS 'hashset', 'int4hashset_init'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_add(int4hashset, int)
RETURNS int4hashset
AS 'hashset', 'int4hashset_add'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_contains(int4hashset, int)
RETURNS boolean
AS 'hashset', 'int4hashset_contains'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_union(int4hashset, int4hashset)
RETURNS int4hashset
AS 'hashset', 'int4hashset_union'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_to_array(int4hashset)
RETURNS int[]
AS 'hashset', 'int4hashset_to_array'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_to_sorted_array(int4hashset)
RETURNS int[]
AS 'hashset', 'int4hashset_to_sorted_array'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_cardinality(int4hashset)
RETURNS bigint
AS 'hashset', 'int4hashset_cardinality'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_capacity(int4hashset)
RETURNS bigint
AS 'hashset', 'int4hashset_capacity'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_collisions(int4hashset)
RETURNS bigint
AS 'hashset', 'int4hashset_collisions'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_max_collisions(int4hashset)
RETURNS bigint
AS 'hashset', 'int4hashset_max_collisions'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION int4_add_int4hashset(int4, int4hashset)
RETURNS int4hashset
AS $$SELECT $2 || $1$$
LANGUAGE SQL
IMMUTABLE PARALLEL SAFE STRICT COST 1;

CREATE OR REPLACE FUNCTION hashset_intersection(int4hashset, int4hashset)
RETURNS int4hashset
AS 'hashset', 'int4hashset_intersection'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_difference(int4hashset, int4hashset)
RETURNS int4hashset
AS 'hashset', 'int4hashset_difference'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_symmetric_difference(int4hashset, int4hashset)
RETURNS int4hashset
AS 'hashset', 'int4hashset_symmetric_difference'
LANGUAGE C IMMUTABLE STRICT;

/*
 * Aggregation Functions
 */

CREATE OR REPLACE FUNCTION int4hashset_agg_add(p_pointer internal, p_value int)
RETURNS internal
AS 'hashset', 'int4hashset_agg_add'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION int4hashset_agg_final(p_pointer internal)
RETURNS int4hashset
AS 'hashset', 'int4hashset_agg_final'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION int4hashset_agg_combine(p_pointer internal, p_pointer2 internal)
RETURNS internal
AS 'hashset', 'int4hashset_agg_combine'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE hashset_agg(int) (
    SFUNC = int4hashset_agg_add,
    STYPE = internal,
    FINALFUNC = int4hashset_agg_final,
    COMBINEFUNC = int4hashset_agg_combine,
    PARALLEL = SAFE
);

CREATE OR REPLACE FUNCTION int4hashset_agg_add_set(p_pointer internal, p_value int4hashset)
RETURNS internal
AS 'hashset', 'int4hashset_agg_add_set'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION int4hashset_agg_final(p_pointer internal)
RETURNS int4hashset
AS 'hashset', 'int4hashset_agg_final'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION int4hashset_agg_combine(p_pointer internal, p_pointer2 internal)
RETURNS internal
AS 'hashset', 'int4hashset_agg_combine'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE hashset_agg(int4hashset) (
    SFUNC = int4hashset_agg_add_set,
    STYPE = internal,
    FINALFUNC = int4hashset_agg_final,
    COMBINEFUNC = int4hashset_agg_combine,
    PARALLEL = SAFE
);

/*
 * Operator Definitions
 */

CREATE OR REPLACE FUNCTION hashset_eq(int4hashset, int4hashset)
RETURNS boolean
AS 'hashset', 'int4hashset_eq'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR = (
    LEFTARG = int4hashset,
    RIGHTARG = int4hashset,
    PROCEDURE = hashset_eq,
    COMMUTATOR = =,
    HASHES
);

CREATE OR REPLACE FUNCTION hashset_ne(int4hashset, int4hashset)
RETURNS boolean
AS 'hashset', 'int4hashset_ne'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <> (
    LEFTARG = int4hashset,
    RIGHTARG = int4hashset,
    PROCEDURE = hashset_ne,
    COMMUTATOR = '<>',
    NEGATOR = '=',
    RESTRICT = neqsel,
    JOIN = neqjoinsel,
    HASHES
);

CREATE OPERATOR || (
    leftarg = int4hashset,
    rightarg = int4,
    function = hashset_add,
    commutator = ||
);

CREATE OPERATOR || (
    leftarg = int4,
    rightarg = int4hashset,
    function = int4_add_int4hashset,
    commutator = ||
);

/*
 * Hashset Hash Operators
 */

CREATE OR REPLACE FUNCTION hashset_hash(int4hashset)
RETURNS integer
AS 'hashset', 'int4hashset_hash'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS int4hashset_hash_ops
DEFAULT FOR TYPE int4hashset USING hash AS
OPERATOR 1 = (int4hashset, int4hashset),
FUNCTION 1 hashset_hash(int4hashset);

/*
 * Hashset Btree Operators
 */

CREATE OR REPLACE FUNCTION hashset_lt(int4hashset, int4hashset)
RETURNS boolean
AS 'hashset', 'int4hashset_lt'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_le(int4hashset, int4hashset)
RETURNS boolean
AS 'hashset', 'int4hashset_le'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_gt(int4hashset, int4hashset)
RETURNS boolean
AS 'hashset', 'int4hashset_gt'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_ge(int4hashset, int4hashset)
RETURNS boolean
AS 'hashset', 'int4hashset_ge'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_cmp(int4hashset, int4hashset)
RETURNS integer
AS 'hashset', 'int4hashset_cmp'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR < (
    PROCEDURE = hashset_lt,
    LEFTARG = int4hashset,
    RIGHTARG = int4hashset,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR <= (
    PROCEDURE = hashset_le,
    LEFTARG = int4hashset,
    RIGHTARG = int4hashset,
    COMMUTATOR = '>=',
    NEGATOR = '>',
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR > (
    PROCEDURE = hashset_gt,
    LEFTARG = int4hashset,
    RIGHTARG = int4hashset,
    COMMUTATOR = '<',
    NEGATOR = '<=',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE OPERATOR >= (
    PROCEDURE = hashset_ge,
    LEFTARG = int4hashset,
    RIGHTARG = int4hashset,
    COMMUTATOR = '<=',
    NEGATOR = '<',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE OPERATOR CLASS int4hashset_btree_ops
DEFAULT FOR TYPE int4hashset USING btree AS
OPERATOR 1 < (int4hashset, int4hashset),
OPERATOR 2 <= (int4hashset, int4hashset),
OPERATOR 3 = (int4hashset, int4hashset),
OPERATOR 4 >= (int4hashset, int4hashset),
OPERATOR 5 > (int4hashset, int4hashset),
FUNCTION 1 hashset_cmp(int4hashset, int4hashset);
