CREATE TABLE IF NOT EXISTS test_hashset_order (hashset_col hashset);
INSERT INTO test_hashset_order (hashset_col) VALUES ('{1,2,3}'::hashset);
INSERT INTO test_hashset_order (hashset_col) VALUES ('{3,2,1}'::hashset);
INSERT INTO test_hashset_order (hashset_col) VALUES ('{4,5,6}'::hashset);
SELECT COUNT(DISTINCT hashset_col) FROM test_hashset_order;

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

SELECT '{2}'::hashset = '{1}'::hashset; -- false
SELECT '{2}'::hashset = '{2}'::hashset; -- true
SELECT '{2}'::hashset = '{3}'::hashset; -- false

SELECT '{2}'::hashset <> '{1}'::hashset; -- true
SELECT '{2}'::hashset <> '{2}'::hashset; -- false
SELECT '{2}'::hashset <> '{3}'::hashset; -- true
