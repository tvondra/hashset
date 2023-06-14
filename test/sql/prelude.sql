CREATE EXTENSION hashset;

CREATE OR REPLACE FUNCTION hashset_sorted(int4hashset)
RETURNS TEXT AS
$$
SELECT array_agg(i ORDER BY i::int)::text
FROM regexp_split_to_table(regexp_replace($1::text,'^{|}$','','g'),',') i
$$ LANGUAGE sql;
