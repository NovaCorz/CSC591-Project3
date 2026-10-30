CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
# common/ is ../common when deployed by scripts/fleet.sh and ../../common inside the repo tree
COMMON := $(firstword $(wildcard ../common ../../common))

ifeq ($(shell uname -m),x86_64)
tlb_test: tlb_test.c $(COMMON)/include/bench_common.h
	$(CC) $(CFLAGS) -I$(COMMON)/include -o $@ tlb_test.c
else
tlb_test:
	@echo "ex08: x86-64 only (this machine is $(shell uname -m)); nothing built"
endif

clean:
	rm -f tlb_test
.PHONY: clean
