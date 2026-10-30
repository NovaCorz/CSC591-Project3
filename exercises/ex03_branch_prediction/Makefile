CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
# common/ is ../common when deployed by scripts/fleet.sh and ../../common inside the repo tree
COMMON := $(firstword $(wildcard ../common ../../common))

ifeq ($(shell uname -m),x86_64)
branch_test: branch_test.c $(COMMON)/include/bench_common.h
	$(CC) $(CFLAGS) -I$(COMMON)/include -o $@ branch_test.c
else
branch_test:
	@echo "ex03: x86-64 only (this machine is $(shell uname -m)); nothing built"
endif

clean:
	rm -f branch_test
.PHONY: clean
