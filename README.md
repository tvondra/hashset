# hashset

This PostgreSQL extension implements hashset, a data structure (type)
providing a collection of unique integer items with fast lookup.

It provides several functions for working with these sets, including operations
like addition, containment check, conversion to array, union, intersection,
difference, equality check, and cardinality calculation.

`NULL` values are also allowed in the hash set, and are considered as a unique
element. When multiple `NULL` values are present in the input, they are treated
as a single `NULL`.

Most functions in this extension are marked as `STRICT`, meaning that if any of
the arguments are `NULL`, then the result is also `NULL`. Exceptions to this
rule are [hashset_add()](#hashset_add) and
[hashset_contains()](#hashset_contains), which have specific behaviors when
handling `NULL`.

## Table of Contents
1. [Version](#version)
2. [Data Types](#data-types)
   - [int4hashset](#int4hashset)
3. [Functions](#functions)
   - [int4hashset](#int4hashset-1)
   - [hashset_add](#hashset_add)
   - [hashset_contains](#hashset_contains)
   - [hashset_to_array](#hashset_to_array)
   - [hashset_to_sorted_array](#hashset_to_sorted_array)
   - [hashset_cardinality](#hashset_cardinality)
   - [hashset_capacity](#hashset_capacity)
   - [hashset_max_collisions](#hashset_max_collisions)
   - [hashset_union](#hashset_union)
   - [hashset_intersection](#hashset_intersection)
   - [hashset_difference](#hashset_difference)
   - [hashset_symmetric_difference](#hashset_symmetric_difference)
4. [Aggregation Functions](#aggregation-functions)
5. [Operators](#operators)
6. [Hashset Hash Operators](#hashset-hash-operators)
7. [Hashset Btree Operators](#hashset-btree-operators)
8. [Limitations](#limitations)
9. [Installation](#installation)
10. [License](#license)

## Version

0.0.1

🚧 **NOTICE** 🚧 This repository is currently under active development and the hashset
PostgreSQL extension is **not production-ready**. As the codebase is evolving
with possible breaking changes, we are not providing any migration scripts
until we reach our first release.


## Data types

### int4hashset

This data type represents a set of integers. Internally, it uses a combination
of a bitmap and a value array to store the elements in a set. It's a
variable-length type.


## Functions

### int4hashset()

`int4hashset([capacity int, load_factor float4, growth_factor float4, hashfn_id int4]) -> int4hashset`

Initialize an empty int4hashset with optional parameters.
  - `capacity` specifies the initial capacity, which is zero by default.
  - `load_factor` represents the threshold for resizing the hashset and defaults to 0.75.
  - `growth_factor` is the multiplier for resizing and defaults to 2.0.
  - `hashfn_id` represents the hash function used.
    - 1=Jenkins/lookup3 (default)
    - 2=MurmurHash32
    - 3=Naive hash function


### hashset_add()

`hashset_add(int4hashset, int) -> int4hashset`

Adds an integer to an int4hashset. The function allows the addition of a `NULL`
value to the set and treats it as a unique element. If the set already contains
a `NULL` value, another `NULL` input is disregarded. If `NULL` is passed as the
first argument, it is treated as an empty set.

```sql
SELECT hashset_add(NULL, 1); -- {1}
SELECT hashset_add('{NULL}', 1); -- {1,NULL}
SELECT hashset_add('{1}', NULL); -- {1,NULL}
SELECT hashset_add('{1}', 1); -- {1}
SELECT hashset_add('{1}', 2); -- {1,2}
```


### hashset_contains()

`hashset_contains(int4hashset, int) -> boolean`

Checks if an int4hashset contains a given integer.

```sql
SELECT hashset_contains('{1}', 1); -- TRUE
SELECT hashset_contains('{1}', 2); -- FALSE
SELECT hashset_contains(NULL, NULL); -- NULL
SELECT hashset_contains(NULL, 1); -- NULL
```

If the *cardinality* of the hashset is zero (0), it is known that it doesn't
contain any value, not even an Unknown value represented as `NULL`, so even in
that case it returns `FALSE`.

```sql
SELECT hashset_contains('{}', 1); -- FALSE
SELECT hashset_contains('{}', NULL); -- FALSE
```


### hashset_to_array()

`hashset_to_array(int4hashset) -> int[]`

Converts an int4hashset to an array of unsorted integers.

```sql
SELECT hashset_to_array('{2,1,3}'); -- {3,2,1}
```


### hashset_to_sorted_array()

`hashset_to_sorted_array(int4hashset) -> int[]`

Converts an int4hashset to an array of sorted integers.

```sql
SELECT hashset_to_sorted_array('{2,1,3}'); -- {1,2,3}
```

If the hashset contains a `NULL` element, it follows the same behavior as the
`ORDER BY` clause in SQL: the `NULL` element is positioned at the end of the
sorted array.

```sql
SELECT hashset_to_sorted_array('{2,1,NULL,3}'); -- {1,2,3,NULL}
```


### hashset_cardinality()

`hashset_cardinality(int4hashset) -> bigint`

Returns the number of elements in an int4hashset.

```sql
SELECT hashset_cardinality(NULL); -- NULL
SELECT hashset_cardinality('{}'); -- 0
SELECT hashset_cardinality('{1}'); -- 1
SELECT hashset_cardinality('{1,1}'); -- 1
SELECT hashset_cardinality('{NULL,NULL}'); -- 1
SELECT hashset_cardinality('{1,NULL}'); -- 2
SELECT hashset_cardinality('{1,2,3}'); -- 3
```


### hashset_capacity()

`hashset_capacity(int4hashset) -> bigint`

Returns the current capacity of an int4hashset.


### hashset_max_collisions()

`hashset_max_collisions(int4hashset) -> bigint`

Returns the maximum number of collisions that have occurred for a single element


### hashset_union()

`hashset_union(int4hashset, int4hashset) -> int4hashset`

Merges two int4hashsets into a new int4hashset.

```sql
SELECT hashset_union('{1,2}', '{2,3}'); -- '{1,2,3}
```


### hashset_intersection()

`hashset_intersection(int4hashset, int4hashset) -> int4hashset`

Returns a new int4hashset that is the intersection of the two input sets.

```sql
SELECT hashset_intersection('{1,2}', '{2,3}'); -- {2}
SELECT hashset_intersection('{1,2,NULL}', '{2,3,NULL}'); -- {2,NULL}
```


### hashset_difference()

`hashset_difference(int4hashset, int4hashset) -> int4hashset`

Returns a new int4hashset that contains the elements present in the first set
but not in the second set.

```sql
SELECT hashset_difference('{1,2}', '{2,3}'); -- {1}
SELECT hashset_difference('{1,2,NULL}', '{2,3,NULL}'); -- {1}
SELECT hashset_difference('{1,2,NULL}', '{2,3}'); -- {1,NULL}
```


### hashset_symmetric_difference()

`hashset_symmetric_difference(int4hashset, int4hashset) -> int4hashset`

Returns a new int4hashset containing elements that are in either of the input sets, but not in their intersection.

```sql
SELECT hashset_symmetric_difference('{1,2}', '{2,3}'); -- {1,3}
SELECT hashset_symmetric_difference('{1,2,NULL}', '{2,3,NULL}'); -- {1,3}
SELECT hashset_symmetric_difference('{1,2,NULL}', '{2,3}'); -- {1,3,NULL}
```


## Aggregation Functions

### hashset_agg(int4)

`hashset_agg(int4) -> int4hashset`

Aggregate integers into a hashset.

```sql
SELECT hashset_agg(some_int4_column) FROM some_table;
```


### hashset_agg(int4hashset)

`hashset_agg(int4hashset) -> int4hashset`

Aggregate hashsets into a hashset.

```sql
SELECT hashset_agg(some_int4hashset_column) FROM some_table;
```


## Operators

- Equality (`=`): Checks if two hashsets are equal.
- Inequality (`<>`): Checks if two hashsets are not equal.


## Hashset Hash Operators

- `hashset_hash(int4hashset) -> integer`: Returns the hash value of an int4hashset.


## Hashset Btree Operators

- `<`, `<=`, `>`, `>=`: Comparison operators for hashsets.


## Limitations

- The `int4hashset` data type currently supports integers within the range of int4
(-2147483648 to 2147483647).


## Installation

To install the extension on any platform, follow these general steps:

1. Ensure you have PostgreSQL installed on your system, including the development files.
2. Clone the repository.
3. Navigate to the cloned repository directory.
4. Compile the extension using `make`.
5. Install the extension using `sudo make install`.
6. Run the tests using `make installcheck` (optional).

To use a different PostgreSQL installation, point configure to a different `pg_config`, using following command:
```sh
make PG_CONFIG=/else/where/pg_config
sudo make install PG_CONFIG=/else/where/pg_config
```

In your PostgreSQL connection, enable the hashset extension using the following SQL command:
```sql
CREATE EXTENSION hashset;
```

This extension requires PostgreSQL version ?.? or later.

For Ubuntu 22.04.1 LTS, you would run the following commands:

```sh
sudo apt install postgresql-15 postgresql-server-dev-15 postgresql-client-15
git clone https://github.com/tvondra/hashset.git
cd hashset
make
sudo make install
make installcheck
```

Please note that this project is currently under active development and is not yet considered production-ready.

## License

This software is distributed under the terms of PostgreSQL license.
See LICENSE or http://www.opensource.org/licenses/bsd-license.php for
more details.
