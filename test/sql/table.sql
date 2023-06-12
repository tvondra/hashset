CREATE TABLE users (
    user_id int PRIMARY KEY,
    user_likes hashset DEFAULT hashset_init(2)
);
INSERT INTO users (user_id) VALUES (1);
UPDATE users SET user_likes = hashset_add(user_likes, 101) WHERE user_id = 1;
UPDATE users SET user_likes = hashset_add(user_likes, 202) WHERE user_id = 1;
SELECT hashset_contains(user_likes, 101) FROM users WHERE user_id = 1;
SELECT hashset_count(user_likes) FROM users WHERE user_id = 1;
SELECT hashset_sorted(user_likes) FROM users WHERE user_id = 1;
