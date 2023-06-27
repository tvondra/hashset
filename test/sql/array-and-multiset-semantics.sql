CREATE OR REPLACE FUNCTION array_union(int4[], int4[])
RETURNS int4[]
AS
$$
SELECT
    CASE
        WHEN
            $1 IS NULL OR $2 IS NULL
        THEN
            NULL
        ELSE
        COALESCE((
            SELECT array_agg(DISTINCT unnest ORDER BY unnest) FROM
            (
                SELECT unnest($1)
                UNION
                SELECT unnest($2)
            ) q
        ),'{}'::int4[])
    END
$$ LANGUAGE sql;

CREATE OR REPLACE FUNCTION array_intersection(int4[], int4[])
RETURNS int4[]
AS
$$
SELECT
    CASE
        WHEN
            $1 IS NULL OR $2 IS NULL
        THEN
            NULL
        ELSE
        COALESCE((
            SELECT array_agg(DISTINCT unnest ORDER BY unnest) FROM
            (
                SELECT unnest($1)
                INTERSECT
                SELECT unnest($2)
            ) q
        ),'{}'::int4[])
    END
$$ LANGUAGE sql;

CREATE OR REPLACE FUNCTION array_difference(int4[], int4[])
RETURNS int4[]
AS
$$
SELECT
    CASE
        WHEN
            $1 IS NULL OR $2 IS NULL
        THEN
            NULL
        ELSE
        COALESCE((
            SELECT array_agg(DISTINCT unnest ORDER BY unnest) FROM
            (
                SELECT unnest($1)
                EXCEPT
                SELECT unnest($2)
            ) q
        ),'{}'::int4[])
    END
$$ LANGUAGE sql;

CREATE OR REPLACE FUNCTION array_symmetric_difference(int4[], int4[])
RETURNS int4[]
AS
$$
SELECT
    CASE
        WHEN
            $1 IS NULL OR $2 IS NULL
        THEN
            NULL
        ELSE
        COALESCE((
            SELECT array_agg(DISTINCT unnest ORDER BY unnest) FROM
            (
                SELECT
                    *
                FROM
                (
                    SELECT unnest($1)
                    UNION
                    SELECT unnest($2)
                ) AS q1
                EXCEPT
                SELECT
                    *
                FROM
                (
                    SELECT unnest($1)
                    INTERSECT
                    SELECT unnest($2)
                ) AS q2
            ) AS q3
        ),'{}'::int4[])
    END
$$ LANGUAGE sql;

CREATE OR REPLACE FUNCTION array_sort_distinct(int4[])
RETURNS int4[]
AS
$$
SELECT
    CASE
        WHEN
            cardinality($1) = 0
        THEN
            '{}'::int4[]
        ELSE
        (
            SELECT array_agg(DISTINCT unnest ORDER BY unnest) FROM unnest($1)
        )
    END
$$ LANGUAGE sql;

DROP TABLE IF EXISTS hashset_test_results_1;
CREATE TABLE hashset_test_results_1 AS
SELECT
    arg1,
    arg2,
    hashset_add(arg1::int4hashset, arg2),
    array_append(arg1::int4[], arg2),
    hashset_contains(arg1::int4hashset, arg2),
    arg2 = ANY(arg1::int4[]) AS "= ANY(...)"
FROM (VALUES (NULL), ('{}'), ('{NULL}'), ('{1}'), ('{2}'), ('{1,2}'), ('{2,3}')) AS a(arg1)
CROSS JOIN (VALUES (NULL::int4), (1::int4), (4::int4)) AS b(arg2);


DROP TABLE IF EXISTS hashset_test_results_2;
CREATE TABLE hashset_test_results_2 AS
SELECT
    arg1,
    arg2,
    hashset_union(arg1::int4hashset, arg2::int4hashset),
    array_union(arg1::int4[], arg2::int4[]),
    hashset_intersection(arg1::int4hashset, arg2::int4hashset),
    array_intersection(arg1::int4[], arg2::int4[]),
    hashset_difference(arg1::int4hashset, arg2::int4hashset),
    array_difference(arg1::int4[], arg2::int4[]),
    hashset_symmetric_difference(arg1::int4hashset, arg2::int4hashset),
    array_symmetric_difference(arg1::int4[], arg2::int4[]),
    hashset_eq(arg1::int4hashset, arg2::int4hashset),
    array_eq(arg1::int4[], arg2::int4[]),
    hashset_ne(arg1::int4hashset, arg2::int4hashset),
    array_ne(arg1::int4[], arg2::int4[])
FROM (VALUES (NULL), ('{}'), ('{NULL}'), ('{1}'), ('{1,NULL}'), ('{2}'), ('{1,2}'), ('{2,3}')) AS a(arg1)
CROSS JOIN (VALUES (NULL), ('{}'), ('{NULL}'), ('{1}'), ('{1,NULL}'), ('{2}'), ('{1,2}'), ('{2,3}')) AS b(arg2);

DROP TABLE IF EXISTS hashset_test_results_3;
CREATE TABLE hashset_test_results_3 AS
SELECT
    arg1,
    hashset_cardinality(arg1::int4hashset),
    cardinality(arg1::int4[])
FROM (VALUES (NULL), ('{}'), ('{NULL}'), ('{1}'), ('{2}'), ('{1,2}'), ('{2,3}')) AS a(arg1);

SELECT * FROM hashset_test_results_1;
SELECT * FROM hashset_test_results_2;
SELECT * FROM hashset_test_results_3;

/*
 * The queries below should not return any rows since the hashset
 * semantics should be identical to array semantics, given the array elements
 * are distinct and both are compared as sorted arrays.
 */

\echo *** Testing: hashset_add()
SELECT * FROM hashset_test_results_1
WHERE
    hashset_to_sorted_array(hashset_add)
IS DISTINCT FROM
    array_sort_distinct(array_append);

\echo *** Testing: hashset_contains()
SELECT * FROM hashset_test_results_1
WHERE
    hashset_contains
IS DISTINCT FROM
    "= ANY(...)";

\echo *** Testing: hashset_union()
SELECT * FROM hashset_test_results_2
WHERE
    hashset_to_sorted_array(hashset_union)
IS DISTINCT FROM
    array_sort_distinct(array_union);

\echo *** Testing: hashset_intersection()
SELECT * FROM hashset_test_results_2
WHERE
    hashset_to_sorted_array(hashset_intersection)
IS DISTINCT FROM
    array_sort_distinct(array_intersection);

\echo *** Testing: hashset_difference()
SELECT * FROM hashset_test_results_2
WHERE
    hashset_to_sorted_array(hashset_difference)
IS DISTINCT FROM
    array_sort_distinct(array_difference);

\echo *** Testing: hashset_symmetric_difference()
SELECT * FROM hashset_test_results_2
WHERE
    hashset_to_sorted_array(hashset_symmetric_difference)
IS DISTINCT FROM
    array_sort_distinct(array_symmetric_difference);

\echo *** Testing: hashset_eq()
SELECT * FROM hashset_test_results_2
WHERE
    hashset_eq
IS DISTINCT FROM
    array_eq;

\echo *** Testing: hashset_ne()
SELECT * FROM hashset_test_results_2
WHERE
    hashset_ne
IS DISTINCT FROM
    array_ne;

\echo *** Testing: hashset_cardinality()
SELECT * FROM hashset_test_results_3
WHERE
    hashset_cardinality
IS DISTINCT FROM
    cardinality;
