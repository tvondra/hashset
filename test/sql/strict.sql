/*
 * Test to verify all relevant functions return NULL if any of
 * the input parameters are NULL, i.e. testing that they are declared as STRICT
 */

SELECT hashset_add(int4hashset(), NULL::int);
SELECT hashset_add(NULL::int4hashset, 123::int);
SELECT hashset_contains('{123,456}'::int4hashset, NULL::int);
SELECT hashset_contains(NULL::int4hashset, 456::int);
SELECT hashset_merge('{1,2}'::int4hashset, NULL::int4hashset);
SELECT hashset_merge(NULL::int4hashset, '{2,3}'::int4hashset);
SELECT hashset_to_array(NULL::int4hashset);
SELECT hashset_count(NULL::int4hashset);
SELECT hashset_capacity(NULL::int4hashset);
SELECT hashset_intersection('{1,2}'::int4hashset,NULL::int4hashset);
SELECT hashset_intersection(NULL::int4hashset,'{2,3}'::int4hashset);
SELECT hashset_difference('{1,2}'::int4hashset,NULL::int4hashset);
SELECT hashset_difference(NULL::int4hashset,'{2,3}'::int4hashset);
SELECT hashset_symmetric_difference('{1,2}'::int4hashset,NULL::int4hashset);
SELECT hashset_symmetric_difference(NULL::int4hashset,'{2,3}'::int4hashset);

/*
 * For convenience, hashset_agg() is not STRICT and just ignore NULL values
 */
SELECT hashset_agg(i) FROM (VALUES (NULL::int),(1::int),(2::int)) q(i);

SELECT hashset_agg(h) FROM
(
    SELECT NULL::int4hashset AS h
    UNION ALL
    SELECT hashset_agg(j) AS h FROM generate_series(6,10) AS j
) q;
