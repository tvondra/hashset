/*
 * This test verifies the hashset input/output functions for varying 
 * initial capacities, ensuring functionality across different sizes.
 */

SELECT hashset_sorted('{1}'::int4hashset);
SELECT hashset_sorted('{1,2}'::int4hashset);
SELECT hashset_sorted('{1,2,3}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13,14}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}'::int4hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}'::int4hashset);
