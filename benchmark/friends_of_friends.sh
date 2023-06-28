#!/bin/sh
if [ ! -f "soc-pokec-relationships.txt" ]; then
    wget https://snap.stanford.edu/data/soc-pokec-relationships.txt.gz
    gunzip soc-pokec-relationships.txt.gz
fi

psql -X -c "CREATE TABLE edges (from_node INT, to_node INT);"
psql -X -c "\COPY edges FROM soc-pokec-relationships.txt;"
psql -X -c "ALTER TABLE edges ADD PRIMARY KEY (from_node, to_node);"
psql -X -f friends_of_friends.sql
