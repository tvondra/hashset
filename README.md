# hashset

This PostgreSQL extension implements hashset, a data structure (type)
providing a collection of integer items with fast lookup.


## Version

0.0.1

🚧 **NOTICE** 🚧 This repository is currently under active development and the hashset
PostgreSQL extension is **not production-ready**. As the codebase is evolving
with possible breaking changes, we are not providing any migration scripts
until we reach our first release.


## Usage

After installing the extension, you can use the `int4hashset` data type and
associated functions within your PostgreSQL queries.

To demonstrate the usage, let's consider a hypothetical table `users` which has
a `user_id` and a `user_likes` of type `int4hashset`.

Firstly, let's create the table:

```sql
CREATE TABLE users(
    user_id int PRIMARY KEY,
    user_likes int4hashset DEFAULT int4hashset()
);
```
In the above statement, the `int4hashset()` initializes an empty hashset
with zero capacity. The hashset will automatically resize itself when more
elements are added.

Now, we can perform operations on this table. Here are some examples:

```sql
-- Insert a new user with id 1. The user_likes will automatically be initialized
-- as an empty hashset
INSERT INTO users (user_id) VALUES (1);

-- Add elements (likes) for a user
UPDATE users SET user_likes = hashset_add(user_likes, 101) WHERE user_id = 1;
UPDATE users SET user_likes = hashset_add(user_likes, 202) WHERE user_id = 1;

-- Check if a user likes a particular item
SELECT hashset_contains(user_likes, 101) FROM users WHERE user_id = 1; -- true

-- Count the number of likes a user has
SELECT hashset_count(user_likes) FROM users WHERE user_id = 1; -- 2
```

You can also use the aggregate functions to perform operations on multiple rows.


## Data types

- **int4hashset**: This data type represents a set of integers. Internally, it uses
a combination of a bitmap and a value array to store the elements in a set. It's
a variable-length type.


## Functions

- `int4hashset() -> int4hashset`: Initialize an empty int4hashset with no capacity.
- `int4hashset_with_capacity(int) -> int4hashset`: Initialize an empty int4hashset with given capacity.
- `hashset_add(int4hashset, int) -> int4hashset`: Adds an integer to an int4hashset.
- `hashset_contains(int4hashset, int) -> boolean`: Checks if an int4hashset contains a given integer.
- `hashset_merge(int4hashset, int4hashset) -> int4hashset`: Merges two int4hashsets into a new int4hashset.
- `hashset_to_array(int4hashset) -> int[]`: Converts an int4hashset to an array of integers.
- `hashset_count(int4hashset) -> bigint`: Returns the number of elements in an int4hashset.
- `hashset_capacity(int4hashset) -> bigint`: Returns the current capacity of an int4hashset.

## Aggregation Functions

- `hashset(int) -> int4hashset`: Aggregate integers into a hashset.
- `hashset(int4hashset) -> int4hashset`: Aggregate hashsets into a hashset.


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

To install the extension, run `make install` in the project root. Then, in your
PostgreSQL connection, execute `CREATE EXTENSION hashset;`.

This extension requires PostgreSQL version ?.? or later.


## License

This software is distributed under the terms of PostgreSQL license.
See LICENSE or http://www.opensource.org/licenses/bsd-license.php for
more details.
