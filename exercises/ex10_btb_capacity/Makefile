CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
# common/ is ../common when deployed by scripts/fleet.sh and ../../common inside the repo tree
COMMON := $(firstword $(wildcard ../common ../../common))

ifeq ($(shell uname -m),x86_64)
btb_test: btb_test.c $(COMMON)/include/bench_common.h
	$(CC) $(CFLAGS) -I$(COMMON)/include -o $@ btb_test.c
else
btb_test:
	@echo "ex10: x86-64 only (this machine is $(shell uname -m)); nothing built"
endif

clean:
	rm -f btb_test
.PHONY: clean
