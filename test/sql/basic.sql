/*
 * Hashset Type
 */

SELECT '{}'::hashset; -- empty hashset
SELECT '{1,2,3}'::hashset;
SELECT '{-2147483648,0,2147483647}'::hashset;
SELECT '{-2147483649}'::hashset; -- out of range
SELECT '{2147483648}'::hashset; -- out of range

/*
 * Hashset Functions
 */

SELECT hashset(); -- init empty hashset with no capacity
SELECT hashset_with_capacity(10); -- init empty hashset with specified capacity
SELECT hashset_add(hashset(), 123);
SELECT hashset_contains('{123,456}'::hashset, 456); -- true
SELECT hashset_contains('{123,456}'::hashset, 789); -- false
SELECT hashset_merge('{1,2}'::hashset, '{2,3}'::hashset);
SELECT hashset_to_array('{1,2,3}'::hashset);
SELECT hashset_count('{1,2,3}'::hashset); -- 3
SELECT hashset_capacity(hashset_with_capacity(10)); -- 10

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

SELECT '{2}'::hashset = '{1}'::hashset; -- false
SELECT '{2}'::hashset = '{2}'::hashset; -- true
SELECT '{2}'::hashset = '{3}'::hashset; -- false

SELECT '{1,2,3}'::hashset = '{1,2,3}'::hashset; -- true
SELECT '{1,2,3}'::hashset = '{2,3,1}'::hashset; -- true
SELECT '{1,2,3}'::hashset = '{4,5,6}'::hashset; -- false
SELECT '{1,2,3}'::hashset = '{1,2}'::hashset; -- false
SELECT '{1,2,3}'::hashset = '{1,2,3,4}'::hashset; -- false

SELECT '{2}'::hashset <> '{1}'::hashset; -- true
SELECT '{2}'::hashset <> '{2}'::hashset; -- false
SELECT '{2}'::hashset <> '{3}'::hashset; -- true

SELECT '{1,2,3}'::hashset <> '{1,2,3}'::hashset; -- false
SELECT '{1,2,3}'::hashset <> '{2,3,1}'::hashset; -- false
SELECT '{1,2,3}'::hashset <> '{4,5,6}'::hashset; -- true
SELECT '{1,2,3}'::hashset <> '{1,2}'::hashset; -- true
SELECT '{1,2,3}'::hashset <> '{1,2,3,4}'::hashset; -- true

/*
 * Hashset Hash Operators
 */

SELECT hashset_hash('{1,2,3}'::hashset);
SELECT hashset_hash('{3,2,1}'::hashset);

SELECT COUNT(*), COUNT(DISTINCT h)
FROM
(
    SELECT '{1,2,3}'::hashset AS h
    UNION ALL
    SELECT '{3,2,1}'::hashset AS h
) q;

/*
 * Hashset Btree Operators
 */

SELECT h FROM
(
    SELECT '{2}'::hashset AS h
    UNION ALL
    SELECT '{1}'::hashset AS h
    UNION ALL
    SELECT '{3}'::hashset AS h
) q
ORDER BY h;

SELECT '{2}'::hashset < '{1}'::hashset; -- false
SELECT '{2}'::hashset < '{2}'::hashset; -- false
SELECT '{2}'::hashset < '{3}'::hashset; -- true

SELECT '{2}'::hashset <= '{1}'::hashset; -- false
SELECT '{2}'::hashset <= '{2}'::hashset; -- true
SELECT '{2}'::hashset <= '{3}'::hashset; -- true

SELECT '{2}'::hashset > '{1}'::hashset; -- true
SELECT '{2}'::hashset > '{2}'::hashset; -- false
SELECT '{2}'::hashset > '{3}'::hashset; -- false

SELECT '{2}'::hashset >= '{1}'::hashset; -- true
SELECT '{2}'::hashset >= '{2}'::hashset; -- true
SELECT '{2}'::hashset >= '{3}'::hashset; -- false
