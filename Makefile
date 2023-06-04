MODULE_big = hashset
OBJS = hashset.o

EXTENSION = hashset
DATA = hashset--1.0.0.sql
MODULES = hashset

CFLAGS=`pg_config --includedir-server`

REGRESS      = basic cast conversions incremental parallel_query value_count_api trimmed_aggregates
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
