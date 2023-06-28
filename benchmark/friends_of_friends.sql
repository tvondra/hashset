CREATE EXTENSION IF NOT EXISTS hashset;

\timing on

CREATE OR REPLACE VIEW vfriends_of_friends_array_agg_distinct AS
WITH RECURSIVE friends_of_friends AS (
    SELECT
        ARRAY[5867::bigint] AS current,
        0 AS depth
    UNION ALL
    SELECT
        new_current,
        friends_of_friends.depth + 1
    FROM
        friends_of_friends
    CROSS JOIN LATERAL (
        SELECT
            array_agg(DISTINCT edges.to_node) AS new_current
        FROM
            edges
        WHERE
            from_node = ANY(friends_of_friends.current)
    ) q
    WHERE
        friends_of_friends.depth < 3
)
SELECT
    COALESCE(array_length(current, 1), 0) AS count_friends_at_depth_3
FROM
    friends_of_friends
WHERE
    depth = 3;

CREATE OR REPLACE VIEW vfriends_of_friends_hashset_agg AS
WITH RECURSIVE friends_of_friends AS
(
    SELECT
        '{5867}'::int4hashset AS current,
        0 AS depth
    UNION ALL
    SELECT
        new_current,
        friends_of_friends.depth + 1
    FROM
        friends_of_friends
    CROSS JOIN LATERAL
    (
        SELECT
            hashset_agg(edges.to_node) AS new_current
        FROM
            edges
        WHERE
            from_node = ANY(hashset_to_array(friends_of_friends.current))
    ) q
    WHERE
        friends_of_friends.depth < 3
)
SELECT
    depth,
    hashset_cardinality(current)
FROM
    friends_of_friends
WHERE
    depth = 3;

SELECT * FROM vfriends_of_friends_array_agg_distinct;
SELECT * FROM vfriends_of_friends_array_agg_distinct;
SELECT * FROM vfriends_of_friends_array_agg_distinct;

SELECT * FROM vfriends_of_friends_hashset_agg;
SELECT * FROM vfriends_of_friends_hashset_agg;
SELECT * FROM vfriends_of_friends_hashset_agg;
