CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -march=native

rob_test: rob_test.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f rob_test
.PHONY: clean
