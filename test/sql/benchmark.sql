DROP EXTENSION IF EXISTS hashset;
CREATE EXTENSION hashset;

\timing on

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
END
$$ LANGUAGE plpgsql;
