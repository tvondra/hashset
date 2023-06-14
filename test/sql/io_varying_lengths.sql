/*
 * This test verifies the hashset input/output functions for varying 
 * initial capacities, ensuring functionality across different sizes.
 */

SELECT hashset_sorted('{1}'::hashset);
SELECT hashset_sorted('{1,2}'::hashset);
SELECT hashset_sorted('{1,2,3}'::hashset);
SELECT hashset_sorted('{1,2,3,4}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13,14}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}'::hashset);
SELECT hashset_sorted('{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}'::hashset);
