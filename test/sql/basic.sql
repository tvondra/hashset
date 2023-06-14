/*
 * Hashset Type
 */

SELECT '{}'::int4hashset; -- empty int4hashset
SELECT '{1,2,3}'::int4hashset;
SELECT '{-2147483648,0,2147483647}'::int4hashset;
SELECT '{-2147483649}'::int4hashset; -- out of range
SELECT '{2147483648}'::int4hashset; -- out of range

/*
 * Hashset Functions
 */

SELECT int4hashset(); -- init empty int4hashset with no capacity
SELECT int4hashset_with_capacity(10); -- init empty int4hashset with specified capacity
SELECT hashset_add(int4hashset(), 123);
SELECT hashset_add(NULL::int4hashset, 123);
SELECT hashset_add('{123}'::int4hashset, 456);
SELECT hashset_contains('{123,456}'::int4hashset, 456); -- true
SELECT hashset_contains('{123,456}'::int4hashset, 789); -- false
SELECT hashset_merge('{1,2}'::int4hashset, '{2,3}'::int4hashset);
SELECT hashset_to_array('{1,2,3}'::int4hashset);
SELECT hashset_count('{1,2,3}'::int4hashset); -- 3
SELECT hashset_capacity(int4hashset_with_capacity(10)); -- 10

/*
 * Aggregation Functions
 */

SELECT hashset(i) FROM generate_series(1,10) AS i;

SELECT hashset(h) FROM
(
    SELECT hashset(i) AS h FROM generate_series(1,5) AS i
    UNION ALL
    SELECT hashset(j) AS h FROM generate_series(6,10) AS j
) q;

/*
 * Operator Definitions
 */

SELECT '{2}'::int4hashset = '{1}'::int4hashset; -- false
SELECT '{2}'::int4hashset = '{2}'::int4hashset; -- true
SELECT '{2}'::int4hashset = '{3}'::int4hashset; -- false

SELECT '{1,2,3}'::int4hashset = '{1,2,3}'::int4hashset; -- true
SELECT '{1,2,3}'::int4hashset = '{2,3,1}'::int4hashset; -- true
SELECT '{1,2,3}'::int4hashset = '{4,5,6}'::int4hashset; -- false
SELECT '{1,2,3}'::int4hashset = '{1,2}'::int4hashset; -- false
SELECT '{1,2,3}'::int4hashset = '{1,2,3,4}'::int4hashset; -- false

SELECT '{2}'::int4hashset <> '{1}'::int4hashset; -- true
SELECT '{2}'::int4hashset <> '{2}'::int4hashset; -- false
SELECT '{2}'::int4hashset <> '{3}'::int4hashset; -- true

SELECT '{1,2,3}'::int4hashset <> '{1,2,3}'::int4hashset; -- false
SELECT '{1,2,3}'::int4hashset <> '{2,3,1}'::int4hashset; -- false
SELECT '{1,2,3}'::int4hashset <> '{4,5,6}'::int4hashset; -- true
SELECT '{1,2,3}'::int4hashset <> '{1,2}'::int4hashset; -- true
SELECT '{1,2,3}'::int4hashset <> '{1,2,3,4}'::int4hashset; -- true

/*
 * Hashset Hash Operators
 */

SELECT hashset_hash('{1,2,3}'::int4hashset);
SELECT hashset_hash('{3,2,1}'::int4hashset);

SELECT COUNT(*), COUNT(DISTINCT h)
FROM
(
    SELECT '{1,2,3}'::int4hashset AS h
    UNION ALL
    SELECT '{3,2,1}'::int4hashset AS h
) q;

/*
 * Hashset Btree Operators
 */

SELECT h FROM
(
    SELECT '{2}'::int4hashset AS h
    UNION ALL
    SELECT '{1}'::int4hashset AS h
    UNION ALL
    SELECT '{3}'::int4hashset AS h
) q
ORDER BY h;

SELECT '{2}'::int4hashset < '{1}'::int4hashset; -- false
SELECT '{2}'::int4hashset < '{2}'::int4hashset; -- false
SELECT '{2}'::int4hashset < '{3}'::int4hashset; -- true

SELECT '{2}'::int4hashset <= '{1}'::int4hashset; -- false
SELECT '{2}'::int4hashset <= '{2}'::int4hashset; -- true
SELECT '{2}'::int4hashset <= '{3}'::int4hashset; -- true

SELECT '{2}'::int4hashset > '{1}'::int4hashset; -- true
SELECT '{2}'::int4hashset > '{2}'::int4hashset; -- false
SELECT '{2}'::int4hashset > '{3}'::int4hashset; -- false

SELECT '{2}'::int4hashset >= '{1}'::int4hashset; -- true
SELECT '{2}'::int4hashset >= '{2}'::int4hashset; -- true
SELECT '{2}'::int4hashset >= '{3}'::int4hashset; -- false
