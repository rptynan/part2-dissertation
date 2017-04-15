LIBALLOCS_BASE = /usr/local/src/liballocs
LIBCRUNCH_BASE := /usr/local/src/libcrunch

# CRUNCHCC = $(LIBCRUNCH_BASE)/frontend/c/bin/crunchcc
CRUNCHCC = /usr/bin/gcc
PLAIN_CC = /usr/bin/gcc
DUMPTYPES = $(LIBALLOCS_BASE)/tools/dumptypes

PLAIN_OBJS = libcrunch.o liballocs.o pageindex.o generic_malloc.o
W_FLAGS = -Wall -Wno-format -std=gnu11
DEBUG_FLAGS = -DDEBUG_CIL_INLINES=2 -DDEBUG_STUBS=2 -DNDEBUG -g


default: all

test.o: test.c
	$(CRUNCHCC) $^ -c -o $@ $(DEBUG_FLAGS) $(W_FLAGS)

%.o: ../allocators/%.c
	$(PLAIN_CC) $^ -I../.. -c -o $@ $(DEBUG_FLAGS) $(W_FLAGS)

%.o: ../%.c
	$(PLAIN_CC) $^ -I../.. -c -o $@ $(DEBUG_FLAGS) $(W_FLAGS)

all: $(PLAIN_OBJS) test.o
	$(DUMPTYPES) test.o > types.c 2> /dev/null
	$(PLAIN_CC) types.c -c -o types.o
	$(PLAIN_CC) $(PLAIN_OBJS) types.o test.o -o test -Wl,-wrap,malloc -lc $(DEBUG_FLAGS)
	./test
