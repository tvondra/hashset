/*
 * In the original implementation of the query, the hashset_add() and
 * hashset_merge() functions were modifying the original hashset in-place.
 * This issue was leading to unexpected results because the functions
 * were altering the original data in the hashset.
 *
 * The problem was fixed by introducing a macro function
 * PG_GETARG_INT4HASHSET_COPY() in the C code. This function ensures that
 * a copy of the hashset is created and modified, leaving the original
 * hashset untouched. This fix resulted in the correct execution of the
 * query, with hashset_add() and hashset_merge() working on the copied
 * hashset, thereby preventing alteration of the original data.
 */
SELECT
    q.hashset,
    hashset_add(hashset,4)
FROM
(
    SELECT
        hashset(generate_series)
    FROM generate_series(1,3)
) q;
