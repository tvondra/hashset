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


CREATE OR REPLACE FUNCTION hashset_add(hashset, int)
    RETURNS hashset
    AS 'hashset', 'hashset_add'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_contains(hashset, int)
    RETURNS bool
    AS 'hashset', 'hashset_contains'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_count(hashset)
    RETURNS bigint
    AS 'hashset', 'hashset_count'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_merge(hashset, hashset)
    RETURNS hashset
    AS 'hashset', 'hashset_merge'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_to_array(hashset)
    RETURNS int[]
    AS 'hashset', 'hashset_to_array'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION hashset_init(int)
    RETURNS hashset
    AS 'hashset', 'hashset_init'
    LANGUAGE C IMMUTABLE;


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
