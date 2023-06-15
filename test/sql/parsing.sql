/* Valid */
SELECT '{1,23,-456}'::int4hashset;
SELECT ' { 1 , 23 , -456 } '::int4hashset;

/* Only whitespace is allowed after the closing brace */
SELECT ' { 1 , 23 , -456 } 1'::int4hashset; -- error
SELECT ' { 1 , 23 , -456 } ,'::int4hashset; -- error
SELECT ' { 1 , 23 , -456 } {'::int4hashset; -- error
SELECT ' { 1 , 23 , -456 } }'::int4hashset; -- error
SELECT ' { 1 , 23 , -456 } x'::int4hashset; -- error

/* Unexpected character when expecting closing brace */
SELECT ' { 1 , 23 , -456 1'::int4hashset; -- error
SELECT ' { 1 , 23 , -456 {'::int4hashset; -- error
SELECT ' { 1 , 23 , -456 x'::int4hashset; -- error

/* Error handling for strtol */
SELECT ' { , 23 , -456 } '::int4hashset; -- error
SELECT ' { 1 , 23 , '::int4hashset; -- error
SELECT ' { s , 23 , -456 } '::int4hashset; -- error

/* Missing opening brace */
SELECT ' 1 , 23 , -456 } '::int4hashset; -- error
