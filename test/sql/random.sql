SELECT setseed(0.12345);

\set MAX_INT 2147483647

CREATE TABLE hashset_random_numbers AS
    SELECT
        (random()*:MAX_INT)::int AS i
    FROM generate_series(1,(random()*10000)::int)
;

SELECT
    md5(hashset_sorted)
FROM
(
    SELECT
        hashset_sorted(hashset(format('{%s}',string_agg(i::text,','))))
    FROM hashset_random_numbers
) q;

SELECT
    md5(input_sorted)
FROM
(
    SELECT
        format('{%s}',string_agg(i::text,',' ORDER BY i)) AS input_sorted
    FROM hashset_random_numbers
) q;
