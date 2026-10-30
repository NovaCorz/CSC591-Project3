CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
# common/ is ../common when deployed by scripts/fleet.sh and ../../common inside the repo tree
COMMON := $(firstword $(wildcard ../common ../../common))

ifeq ($(shell uname -m),x86_64)
ooo_test: ooo_test.c $(COMMON)/include/bench_common.h
	$(CC) $(CFLAGS) -I$(COMMON)/include -o $@ ooo_test.c
else
ooo_test:
	@echo "ex02: x86-64 only (this machine is $(shell uname -m)); nothing built"
endif

clean:
	rm -f ooo_test
.PHONY: clean
