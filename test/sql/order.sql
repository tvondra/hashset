CREATE TABLE IF NOT EXISTS test_int4hashset_order (int4hashset_col int4hashset);
INSERT INTO test_int4hashset_order (int4hashset_col) VALUES ('{1,2,3}'::int4hashset);
INSERT INTO test_int4hashset_order (int4hashset_col) VALUES ('{3,2,1}'::int4hashset);
INSERT INTO test_int4hashset_order (int4hashset_col) VALUES ('{4,5,6}'::int4hashset);
SELECT COUNT(DISTINCT int4hashset_col) FROM test_int4hashset_order;

CREATE OR REPLACE FUNCTION generate_random_int4hashset(num_elements INT)
RETURNS int4hashset AS $$
DECLARE
  element INT;
  random_set int4hashset;
BEGIN
  random_set := int4hashset_with_capacity(num_elements);

  FOR i IN 1..num_elements LOOP
    element := floor(random() * 1000)::INT;
    random_set := hashset_add(random_set, element);
  END LOOP;

  RETURN random_set;
END;
$$ LANGUAGE plpgsql;

SELECT setseed(0.123465);

CREATE TABLE int4hashset_order_test AS
SELECT generate_random_int4hashset(3) AS hashset_col
FROM generate_series(1,1000)
UNION
SELECT generate_random_int4hashset(2)
FROM generate_series(1,1000);

SELECT hashset_col
FROM int4hashset_order_test
ORDER BY hashset_col
LIMIT 20;
