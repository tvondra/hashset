MODULE_big = hashset
OBJS = hashset.o

EXTENSION = hashset
DATA = hashset--0.0.1.sql
MODULES = hashset

CFLAGS=`pg_config --includedir-server`

REGRESS = prelude basic random table invalid

REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
