# hashset

This PostgreSQL extension implements hashset, a data structure (type)
providing a collection of unique, not null integer items with fast lookup.


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
SELECT hashset_cardinality(user_likes) FROM users WHERE user_id = 1; -- 2
```

You can also use the aggregate functions to perform operations on multiple rows.


## Data types

- **int4hashset**: This data type represents a set of integers. Internally, it uses
a combination of a bitmap and a value array to store the elements in a set. It's
a variable-length type.


## Functions

- `int4hashset([capacity int, load_factor float4, growth_factor float4, hashfn_id int4]) -> int4hashset`:
  Initialize an empty int4hashset with optional parameters.
    - `capacity` specifies the initial capacity, which is zero by default.
    - `load_factor` represents the threshold for resizing the hashset and defaults to 0.75.
    - `growth_factor` is the multiplier for resizing and defaults to 2.0.
    - `hashfn_id` represents the hash function used.
        - 1=Jenkins/lookup3 (default)
        - 2=MurmurHash32
        - 3=Naive hash function
- `hashset_add(int4hashset, int) -> int4hashset`: Adds an integer to an int4hashset.
- `hashset_contains(int4hashset, int) -> boolean`: Checks if an int4hashset contains a given integer.
- `hashset_union(int4hashset, int4hashset) -> int4hashset`: Merges two int4hashsets into a new int4hashset.
- `hashset_to_array(int4hashset) -> int[]`: Converts an int4hashset to an array of integers.
- `hashset_cardinality(int4hashset) -> bigint`: Returns the number of elements in an int4hashset.
- `hashset_capacity(int4hashset) -> bigint`: Returns the current capacity of an int4hashset.
- `hashset_max_collisions(int4hashset) -> bigint`: Returns the maximum number of collisions that have occurred for a single element
- `hashset_intersection(int4hashset, int4hashset) -> int4hashset`: Returns a new int4hashset that is the intersection of the two input sets.
- `hashset_difference(int4hashset, int4hashset) -> int4hashset`: Returns a new int4hashset that contains the elements present in the first set but not in the second set.
- `hashset_symmetric_difference(int4hashset, int4hashset) -> int4hashset`: Returns a new int4hashset containing elements that are in either of the input sets, but not in their intersection.

## Aggregation Functions

- `hashset_agg(int) -> int4hashset`: Aggregate integers into a hashset.
- `hashset_agg(int4hashset) -> int4hashset`: Aggregate hashsets into a hashset.


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
