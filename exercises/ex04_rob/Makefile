CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS_EXTRA ?=
# common/ is ../common when deployed by scripts/fleet.sh and ../../common inside the repo tree
COMMON := $(firstword $(wildcard ../common ../../common))

rob_test: rob_test.c $(COMMON)/include/bench_common.h
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -I$(COMMON)/include -o $@ rob_test.c

clean:
	rm -f rob_test
.PHONY: clean
