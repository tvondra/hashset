# hashset

This PostgreSQL extension implements hashset, a data structure (type)
providing a collection of integer items with fast lookup.

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

The extension provides the following functions:

### hashset_add(hashset, int) -> hashset
Adds an integer to a `hashset`.

### hashset_contains(hashset, int) -> boolean
Checks if an integer is contained in a `hashset`.

### hashset_count(hashset) -> bigint
Returns the number of elements in a `hashset`.

### hashset_merge(hashset, hashset) -> hashset
Merges two `hashset`s into a single `hashset`.

### hashset_to_array(hashset) -> integer[]
Converts a `hashset` to an integer array.

### hashset_init(int) -> hashset
Initializes an empty `hashset` with a specified initial capacity for maximum
elements. The argument determines the maximum number of elements the `hashset`
can hold before it needs to resize.

## Aggregate Functions

### hashset(integer) -> hashset
Generates a `hashset` from a series of integers, keeping only the unique ones.

### hashset(hashset) -> hashset
Merges multiple `hashset`s into a single `hashset`, preserving unique elements.


## Installation

To install the extension, run `make install` in the project root. Then, in your
PostgreSQL connection, execute `CREATE EXTENSION hashset;`.

This extension requires PostgreSQL version ?.? or later.


## License

This software is distributed under the terms of PostgreSQL license.
See LICENSE or http://www.opensource.org/licenses/bsd-license.php for
more details.
