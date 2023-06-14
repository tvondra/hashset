CREATE TABLE IF NOT EXISTS test_hashset_order (hashset_col hashset);
INSERT INTO test_hashset_order (hashset_col) VALUES ('{1,2,3}'::hashset);
INSERT INTO test_hashset_order (hashset_col) VALUES ('{3,2,1}'::hashset);
INSERT INTO test_hashset_order (hashset_col) VALUES ('{4,5,6}'::hashset);
SELECT COUNT(DISTINCT hashset_col) FROM test_hashset_order;

CREATE OR REPLACE FUNCTION generate_random_hashset(num_elements INT)
RETURNS hashset AS $$
DECLARE
  element INT;
  random_set hashset;
BEGIN
  random_set := hashset_init(num_elements);

  FOR i IN 1..num_elements LOOP
    element := floor(random() * 1000)::INT;
    random_set := hashset_add(random_set, element);
  END LOOP;

  RETURN random_set;
END;
$$ LANGUAGE plpgsql;

SELECT setseed(0.123465);

CREATE TABLE hashset_order_test AS
SELECT generate_random_hashset(3) AS hashset_col
FROM generate_series(1,1000)
UNION
SELECT generate_random_hashset(2)
FROM generate_series(1,1000);

SELECT hashset_col
FROM hashset_order_test
ORDER BY hashset_col
LIMIT 20;
