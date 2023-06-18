DROP EXTENSION IF EXISTS hashset;
CREATE EXTENSION hashset;

\timing on

\echo * Benchmark array_agg(DISTINCT ...) vs hashset_agg()

DROP TABLE IF EXISTS benchmark_input_100k;
DROP TABLE IF EXISTS benchmark_input_10M;
DROP TABLE IF EXISTS benchmark_array_agg;
DROP TABLE IF EXISTS benchmark_hashset_agg;

CREATE TABLE benchmark_input_100k AS
SELECT
    i,
    i/10 AS j,
    (floor(4294967296 * random()) - 2147483648)::int AS rnd
FROM generate_series(1,100000) AS i;

CREATE TABLE benchmark_input_10M AS
SELECT
    i,
    i/10 AS j,
    (floor(4294967296 * random()) - 2147483648)::int AS rnd
FROM generate_series(1,10000000) AS i;

\echo *** Benchmark array_agg(DISTINCT ...) vs hashset_agg(...) for 100k unique integers
CREATE TABLE benchmark_array_agg AS
SELECT array_agg(DISTINCT i) FROM benchmark_input_100k;
CREATE TABLE benchmark_hashset_agg AS
SELECT hashset_agg(i) FROM benchmark_input_100k;

\echo *** Benchmark array_agg(DISTINCT ...) vs hashset_agg(...) for 10M unique integers
INSERT INTO benchmark_array_agg
SELECT array_agg(DISTINCT i) FROM benchmark_input_10M;
INSERT INTO benchmark_hashset_agg
SELECT hashset_agg(i) FROM benchmark_input_10M;

\echo *** Benchmark array_agg(DISTINCT ...) vs hashset_agg(...) for 100k integers (10% uniqueness)
INSERT INTO benchmark_array_agg
SELECT array_agg(DISTINCT j) FROM benchmark_input_100k;
INSERT INTO benchmark_hashset_agg
SELECT hashset_agg(j) FROM benchmark_input_100k;

\echo *** Benchmark array_agg(DISTINCT ...) vs hashset_agg(...) for 10M integers (10% uniqueness)
INSERT INTO benchmark_array_agg
SELECT array_agg(DISTINCT j) FROM benchmark_input_10M;
INSERT INTO benchmark_hashset_agg
SELECT hashset_agg(j) FROM benchmark_input_10M;

\echo *** Benchmark array_agg(DISTINCT ...) vs hashset_agg(...) for 100k random integers
INSERT INTO benchmark_array_agg
SELECT array_agg(DISTINCT rnd) FROM benchmark_input_100k;
INSERT INTO benchmark_hashset_agg
SELECT hashset_agg(rnd) FROM benchmark_input_100k;

\echo *** Benchmark array_agg(DISTINCT ...) vs hashset_agg(...) for 10M random integers
INSERT INTO benchmark_array_agg
SELECT array_agg(DISTINCT rnd) FROM benchmark_input_10M;
INSERT INTO benchmark_hashset_agg
SELECT hashset_agg(rnd) FROM benchmark_input_10M;

SELECT cardinality(array_agg) FROM benchmark_array_agg ORDER BY 1;

SELECT
    hashset_count(hashset_agg),
    hashset_capacity(hashset_agg),
    hashset_collisions(hashset_agg),
    hashset_max_collisions(hashset_agg)
FROM benchmark_hashset_agg;

SELECT hashset_capacity(hashset_agg(rnd)) FROM benchmark_input_10M;

\echo * Benchmark different hash functions

\echo *** Elements in sequence 1..100000

\echo - Testing default hash function (Jenkins/lookup3)

DO
$$
DECLARE
    h int4hashset;
BEGIN
    h := int4hashset(hashfn_id := 1);
    FOR i IN 1..100000 LOOP
        h := hashset_add(h, i);
    END LOOP;
    RAISE NOTICE 'hashset_count: %', hashset_count(h);
    RAISE NOTICE 'hashset_capacity: %', hashset_capacity(h);
    RAISE NOTICE 'hashset_collisions: %', hashset_collisions(h);
    RAISE NOTICE 'hashset_max_collisions: %', hashset_max_collisions(h);
END
$$ LANGUAGE plpgsql;

\echo - Testing Murmurhash32

DO
$$
DECLARE
    h int4hashset;
BEGIN
    h := int4hashset(hashfn_id := 2);
    FOR i IN 1..100000 LOOP
        h := hashset_add(h, i);
    END LOOP;
    RAISE NOTICE 'hashset_count: %', hashset_count(h);
    RAISE NOTICE 'hashset_capacity: %', hashset_capacity(h);
    RAISE NOTICE 'hashset_collisions: %', hashset_collisions(h);
    RAISE NOTICE 'hashset_max_collisions: %', hashset_max_collisions(h);
END
$$ LANGUAGE plpgsql;

\echo - Testing naive hash function

DO
$$
DECLARE
    h int4hashset;
BEGIN
    h := int4hashset(hashfn_id := 3);
    FOR i IN 1..100000 LOOP
        h := hashset_add(h, i);
    END LOOP;
    RAISE NOTICE 'hashset_count: %', hashset_count(h);
    RAISE NOTICE 'hashset_capacity: %', hashset_capacity(h);
    RAISE NOTICE 'hashset_collisions: %', hashset_collisions(h);
    RAISE NOTICE 'hashset_max_collisions: %', hashset_max_collisions(h);
END
$$ LANGUAGE plpgsql;

\echo *** Testing 100000 random ints

SELECT setseed(0.12345);
\echo - Testing default hash function (Jenkins/lookup3)

DO
$$
DECLARE
    h int4hashset;
BEGIN
    h := int4hashset(hashfn_id := 1);
    FOR i IN 1..100000 LOOP
        h := hashset_add(h, (floor(4294967296 * random()) - 2147483648)::int);
    END LOOP;
    RAISE NOTICE 'hashset_count: %', hashset_count(h);
    RAISE NOTICE 'hashset_capacity: %', hashset_capacity(h);
    RAISE NOTICE 'hashset_collisions: %', hashset_collisions(h);
    RAISE NOTICE 'hashset_max_collisions: %', hashset_max_collisions(h);
END
$$ LANGUAGE plpgsql;

SELECT setseed(0.12345);
\echo - Testing Murmurhash32

DO
$$
DECLARE
    h int4hashset;
BEGIN
    h := int4hashset(hashfn_id := 2);
    FOR i IN 1..100000 LOOP
        h := hashset_add(h, (floor(4294967296 * random()) - 2147483648)::int);
    END LOOP;
    RAISE NOTICE 'hashset_count: %', hashset_count(h);
    RAISE NOTICE 'hashset_capacity: %', hashset_capacity(h);
    RAISE NOTICE 'hashset_collisions: %', hashset_collisions(h);
    RAISE NOTICE 'hashset_max_collisions: %', hashset_max_collisions(h);
END
$$ LANGUAGE plpgsql;

SELECT setseed(0.12345);
\echo - Testing naive hash function

DO
$$
DECLARE
    h int4hashset;
BEGIN
    h := int4hashset(hashfn_id := 3);
    FOR i IN 1..100000 LOOP
        h := hashset_add(h, (floor(4294967296 * random()) - 2147483648)::int);
    END LOOP;
    RAISE NOTICE 'hashset_count: %', hashset_count(h);
    RAISE NOTICE 'hashset_capacity: %', hashset_capacity(h);
    RAISE NOTICE 'hashset_collisions: %', hashset_collisions(h);
    RAISE NOTICE 'hashset_max_collisions: %', hashset_max_collisions(h);
END
$$ LANGUAGE plpgsql;
