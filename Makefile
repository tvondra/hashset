MODULE_big = hashset
OBJS = hashset.o hashset-api.o

EXTENSION = hashset
DATA = hashset--0.0.1.sql
MODULES = hashset

# Keep the CFLAGS separate
SERVER_INCLUDES=-I$(shell pg_config --includedir-server)
CLIENT_INCLUDES=-I$(shell pg_config --includedir)
LIBRARY_PATH = -L$(shell pg_config --libdir)

REGRESS = prelude basic io_varying_lengths random table invalid parsing reported_bugs
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

C_TESTS_DIR = test/c_tests

EXTRA_CLEAN = $(C_TESTS_DIR)/test_send_recv

c_tests: $(C_TESTS_DIR)/test_send_recv

$(C_TESTS_DIR)/test_send_recv: $(C_TESTS_DIR)/test_send_recv.c
	$(CC) $(SERVER_INCLUDES) $(CLIENT_INCLUDES) -o $@ $< $(LIBRARY_PATH) -lpq

run_c_tests: c_tests
	cd $(C_TESTS_DIR) && ./test_send_recv.sh

check: all $(REGRESS_PREP) run_c_tests

include $(PGXS)
