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

After installing the extension, you can use the `hashset` data type and
associated functions within your PostgreSQL queries.

To demonstrate the usage, let's consider a hypothetical table `users` which has
a `user_id` and a `user_likes` of type `hashset`.

Firstly, let's create the table:

```sql
CREATE TABLE users(
    user_id int PRIMARY KEY,
    user_likes hashset DEFAULT hashset_init(2)
);
```
In the above statement, the `hashset_init(2)` initializes a hashset with initial
capacity for 2 elements. The hashset will automatically resize itself when more
elements are added beyond this initial capacity.

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
For instance, you can add an integer to a `hashset`.


## Data types

- **hashset**: This data type represents a set of integers. Internally, it uses
a combination of a bitmap and a value array to store the elements in a set. It's
a variable-length type.


## Functions

- `hashset_init(int) -> hashset`: Initialize an empty hashset.
- `hashset_add(hashset, int) -> hashset`: Adds an integer to a hashset.
- `hashset_contains(hashset, int) -> boolean`: Checks if a hashset contains a given integer.
- `hashset_merge(hashset, hashset) -> hashset`: Merges two hashsets into a new hashset.
- `hashset_to_array(hashset) -> int[]`: Converts a hashset to an array of integers.
- `hashset_count(hashset) -> bigint`: Returns the number of elements in a hashset.


## Aggregation Functions

- `hashset(int) -> hashset`: Aggregate integers into a hashset.
- `hashset(hashset) -> hashset`: Aggregate hashsets into a hashset.


## Operators

- Equality (`=`): Checks if two hashsets are equal.
- Inequality (`<>`): Checks if two hashsets are not equal.


## Hashset Hash Operators

- `hashset_hash(hashset) -> integer`: Returns the hash value of a hashset.


## Hashset Btree Operators

- `<`, `<=`, `>`, `>=`: Comparison operators for hashsets.


## Limitations

- The `hashset` data type currently supports integers within the range of int4
(-2147483648 to 2147483647).


## Installation

To install the extension, run `make install` in the project root. Then, in your
PostgreSQL connection, execute `CREATE EXTENSION hashset;`.

This extension requires PostgreSQL version ?.? or later.


## License

This software is distributed under the terms of PostgreSQL license.
See LICENSE or http://www.opensource.org/licenses/bsd-license.php for
more details.
