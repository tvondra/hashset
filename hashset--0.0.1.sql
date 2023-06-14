/*
 * Hashset Type Definition
 */

CREATE TYPE hashset;

CREATE OR REPLACE FUNCTION hashset_in(cstring)
    RETURNS hashset
    AS 'hashset', 'hashset_in'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_out(hashset)
    RETURNS cstring
    AS 'hashset', 'hashset_out'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_send(hashset)
    RETURNS bytea
    AS 'hashset', 'hashset_send'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_recv(internal)
    RETURNS hashset
    AS 'hashset', 'hashset_recv'
    LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE hashset (
    INPUT = hashset_in,
    OUTPUT = hashset_out,
    RECEIVE = hashset_recv,
    SEND = hashset_send,
    INTERNALLENGTH = variable,
    STORAGE = external
);

/*
 * Hashset Functions
 */

CREATE OR REPLACE FUNCTION hashset_init(int)
    RETURNS hashset
    AS 'hashset', 'hashset_init'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_add(hashset, int)
    RETURNS hashset
    AS 'hashset', 'hashset_add'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_contains(hashset, int)
    RETURNS bool
    AS 'hashset', 'hashset_contains'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_merge(hashset, hashset)
    RETURNS hashset
    AS 'hashset', 'hashset_merge'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_to_array(hashset)
    RETURNS int[]
    AS 'hashset', 'hashset_to_array'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_count(hashset)
    RETURNS bigint
    AS 'hashset', 'hashset_count'
    LANGUAGE C IMMUTABLE;


/*
 * Aggregation Functions
 */

CREATE OR REPLACE FUNCTION hashset_agg_add(p_pointer internal, p_value int)
    RETURNS internal
    AS 'hashset', 'hashset_agg_add'
    LANGUAGE C IMMUTABLE;
    
CREATE OR REPLACE FUNCTION hashset_agg_final(p_pointer internal)
    RETURNS hashset
    AS 'hashset', 'hashset_agg_final'
    LANGUAGE C IMMUTABLE;
    
CREATE OR REPLACE FUNCTION hashset_agg_combine(p_pointer internal, p_pointer2 internal)
    RETURNS internal
    AS 'hashset', 'hashset_agg_combine'
    LANGUAGE C IMMUTABLE;

CREATE AGGREGATE hashset(int) (
    SFUNC = hashset_agg_add,
    STYPE = internal,
    FINALFUNC = hashset_agg_final,
    COMBINEFUNC = hashset_agg_combine,
    PARALLEL = SAFE
);

CREATE OR REPLACE FUNCTION hashset_agg_add_set(p_pointer internal, p_value hashset)
    RETURNS internal
    AS 'hashset', 'hashset_agg_add_set'
    LANGUAGE C IMMUTABLE;
    
CREATE OR REPLACE FUNCTION hashset_agg_final(p_pointer internal)
    RETURNS hashset
    AS 'hashset', 'hashset_agg_final'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_agg_combine(p_pointer internal, p_pointer2 internal)
    RETURNS internal
    AS 'hashset', 'hashset_agg_combine'
    LANGUAGE C IMMUTABLE;

CREATE AGGREGATE hashset(hashset) (
    SFUNC = hashset_agg_add_set,
    STYPE = internal,
    FINALFUNC = hashset_agg_final,
    COMBINEFUNC = hashset_agg_combine,
    PARALLEL = SAFE
);

/*
 * Operator Definitions
 */

CREATE OR REPLACE FUNCTION hashset_equals(hashset, hashset)
    RETURNS bool
    AS 'hashset', 'hashset_equals'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR = (
    LEFTARG = hashset,
    RIGHTARG = hashset,
    PROCEDURE = hashset_equals,
    COMMUTATOR = =,
    HASHES
);

CREATE OR REPLACE FUNCTION hashset_neq(hashset, hashset)
    RETURNS bool
    AS 'hashset', 'hashset_neq'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <> (
    LEFTARG = hashset,
    RIGHTARG = hashset,
    PROCEDURE = hashset_neq,
    COMMUTATOR = '<>',
    NEGATOR = '=',
    RESTRICT = neqsel,
    JOIN = neqjoinsel,
    HASHES
);

/*
 * Hashset Hash Operators
 */

CREATE OR REPLACE FUNCTION hashset_hash(hashset)
    RETURNS integer
    AS 'hashset', 'hashset_hash'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS hashset_hash_ops
    DEFAULT FOR TYPE hashset USING hash AS
    OPERATOR 1 = (hashset, hashset),
    FUNCTION 1 hashset_hash(hashset);

/*
 * Hashset Btree Operators
 */

CREATE OR REPLACE FUNCTION hashset_lt(hashset, hashset)
    RETURNS bool
    AS 'hashset', 'hashset_lt'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_le(hashset, hashset)
    RETURNS boolean
    AS 'hashset', 'hashset_le'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_gt(hashset, hashset)
    RETURNS boolean
    AS 'hashset', 'hashset_gt'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_ge(hashset, hashset)
    RETURNS boolean
    AS 'hashset', 'hashset_ge'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION hashset_cmp(hashset, hashset)
    RETURNS integer
    AS 'hashset', 'hashset_cmp'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR < (
    LEFTARG = hashset,
    RIGHTARG = hashset,
    PROCEDURE = hashset_lt,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR <= (
    PROCEDURE = hashset_le,
    LEFTARG = hashset,
    RIGHTARG = hashset,
    COMMUTATOR = '>=',
    NEGATOR = '>',
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR > (
    PROCEDURE = hashset_gt,
    LEFTARG = hashset,
    RIGHTARG = hashset,
    COMMUTATOR = '<',
    NEGATOR = '<=',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE OPERATOR >= (
    PROCEDURE = hashset_ge,
    LEFTARG = hashset,
    RIGHTARG = hashset,
    COMMUTATOR = '<=',
    NEGATOR = '<',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE OPERATOR CLASS hashset_btree_ops
    DEFAULT FOR TYPE hashset USING btree AS
    OPERATOR 1 < (hashset, hashset),
    OPERATOR 2 <= (hashset, hashset),
    OPERATOR 3 = (hashset, hashset),
    OPERATOR 4 >= (hashset, hashset),
    OPERATOR 5 > (hashset, hashset),
    FUNCTION 1 hashset_cmp(hashset, hashset);
