/*
 * Bug in hashset_add() and hashset_merge() functions altering original hashset.
 *
 * Previously, the hashset_add() and hashset_merge() functions were modifying the
 * original hashset in-place, leading to unexpected results as the original data
 * within the hashset was being altered.
 *
 * The issue was addressed by implementing a macro function named
 * PG_GETARG_INT4HASHSET_COPY() within the C code. This function guarantees that
 * a copy of the hashset is created and subsequently modified, thereby preserving
 * the integrity of the original hashset.
 *
 * As a result of this fix, hashset_add() and hashset_merge() now operate on
 * a copied hashset, ensuring that the original data remains unaltered, and
 * the query executes correctly.
 */
SELECT
    q.hashset_agg,
    hashset_add(hashset_agg,4)
FROM
(
    SELECT
        hashset_agg(generate_series)
    FROM generate_series(1,3)
) q;

/*
 * Bug in hashset_hash() function with respect to element insertion order.
 *
 * Prior to the fix, the hashset_hash() function was accumulating the hashes
 * of individual elements in a non-commutative manner. As a consequence, the
 * final hash value was sensitive to the order in which elements were inserted
 * into the hashset. This behavior led to inconsistencies, as logically
 * equivalent sets (i.e., sets with the same elements but in different orders)
 * produced different hash values.
 *
 * The bug was fixed by modifying the hashset_hash() function to use a
 * commutative operation when combining the hashes of individual elements.
 * This change ensures that the final hash value is independent of the
 * element insertion order, and logically equivalent sets produce the
 * same hash.
 */
SELECT hashset_hash('{1,2}'::int4hashset);
SELECT hashset_hash('{2,1}'::int4hashset);

SELECT hashset_cmp('{1,2}','{2,1}')
UNION
SELECT hashset_cmp('{1,2}','{1,2,1}')
UNION
SELECT hashset_cmp('{1,2}','{1,2}');

/*
 * Bug in int4hashset_resize() not utilizing growth_factor.
 *
 * The previous implementation hard-coded a growth factor of 2, neglecting
 * the struct's growth_factor field. This bug was addressed by properly
 * using growth_factor for new capacity calculation, with an additional
 * safety check to prevent possible infinite loops in resizing.
 */
SELECT hashset_capacity(hashset_add(hashset_add(int4hashset(
    capacity := 0,
    load_factor := 0.75,
    growth_factor := 1.1
), 123), 456));

SELECT hashset_capacity(hashset_add(hashset_add(int4hashset(
    capacity := 0,
    load_factor := 0.75,
    growth_factor := 10
), 123), 456));
