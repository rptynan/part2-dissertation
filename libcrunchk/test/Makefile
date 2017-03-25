export LIBALLOCS_BASE = /usr/local/src/liballocs
export LIBCRUNCH_BASE := /usr/local/src/libcrunch
export LIBCRUNCH_ALWAYS_PRINT_EXIT_SUMMARY := 1

CRUNCHCC ?= $(LIBCRUNCH_BASE)/frontend/c/bin/crunchcc
PLAIN_CC = /usr/bin/gcc
CC = $(CRUNCHCC)
export CRUNCHCC
export PLAIN_CC
export CC

export DUMPTYPES = $(LIBALLOCS_BASE)/tools/dumptypes

default: all

all:
	$(CRUNCHCC) test.c -c -o test.o
	$(DUMPTYPES) test.o > types.c 2> /dev/null
	$(PLAIN_CC) types.c -c -o types.o
	$(PLAIN_CC) ../stubs.c types.o test.o -o test -DDEBUG_STUBS
	./test
