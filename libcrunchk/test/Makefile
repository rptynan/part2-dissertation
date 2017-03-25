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

export BUILD_DIR = ./build

default: all

all:
	mkdir -p $(BUILD_DIR)
	$(CRUNCHCC) test.c -c -o $(BUILD_DIR)/test.o -DDEBUG_CIL_INLINES=2
	$(DUMPTYPES) $(BUILD_DIR)/test.o > $(BUILD_DIR)/types.c 2> /dev/null
	$(PLAIN_CC) $(BUILD_DIR)/types.c -c -o $(BUILD_DIR)/types.o
	$(PLAIN_CC) ../stubs.c $(BUILD_DIR)/types.o $(BUILD_DIR)/test.o -o $(BUILD_DIR)/test -DDEBUG_STUBS=2 -Wl,-wrap,malloc
	./$(BUILD_DIR)/test
