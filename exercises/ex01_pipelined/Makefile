CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
# common/ is ../common when deployed by scripts/fleet.sh and ../../common inside the repo tree
COMMON := $(firstword $(wildcard ../common ../../common))

ifeq ($(shell uname -m),x86_64)
pipe_test: pipe_test.c $(COMMON)/include/bench_common.h
	$(CC) $(CFLAGS) -I$(COMMON)/include -o $@ pipe_test.c
else
pipe_test:
	@echo "ex01: x86-64 only (this machine is $(shell uname -m)); nothing built"
endif

clean:
	rm -f pipe_test
.PHONY: clean
