LIBALLOCS_BASE = /usr/local/src/liballocs
LIBCRUNCH_BASE := /usr/local/src/libcrunch
LIBCRUNCHK_BASE := /usr/local/src/part2-dissertation-code/libcrunchk

CRUNCHCC ?= $(LIBCRUNCH_BASE)/frontend/c/bin/crunchcc
# CRUNCHCC = /usr/bin/gcc
PLAIN_CC = /usr/bin/gcc
DUMPTYPES = $(LIBALLOCS_BASE)/tools/dumptypes
TYPESSPLITTER = python2 $(LIBCRUNCHK_BASE)/../scripts/typessplitter/split.py

LIBCRUNCH_OBJS = libcrunch.o liballocs.o pageindex.o generic_malloc.o static.o index_tree.o
W_FLAGS = -Wall -Wno-format -std=gnu11
DEBUG_FLAGS = -DDEBUG_CIL_INLINES=2 -DDEBUG_STUBS=2 -g
WRAP_FLAGS ?= -Wl,-wrap,malloc,-wrap,free,-wrap,reallocf,-T,../ldscript

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
MAKE = gmake
INCLUDE_MK_INC = `if test -e $(dir $(realpath $(THIS_MAKEFILE)))/$*/mk.inc; then echo -f mk.inc; else true; fi`
IS_INTERNAL_TEST ?= 0
RERUN_TESTS_ON_CHANGE =  # Just blanking out from original test makefile


default: checkall
checkall: clear-stamps checkrun-my-test checkrun-index-tree-test \
	checkrun-hello-heap checkrun-hello-array checkrun-hello-errno \
	checkrun-hello-indirect checkrun-hello-funptr
clear-stamps:  # Don't know how to rm these properly
	find . -name "*-stamp" | xargs rm -f

# This rule and the one below for building LIBCRUNCH_OBJS from test dirs
%.o: ../../allocators/%.c
	$(PLAIN_CC) $^ -I../../.. -c -o $@ $(DEBUG_FLAGS) $(W_FLAGS)
%.o: ../../%.c
	$(PLAIN_CC) $^ -I../../.. -c -o $@ $(DEBUG_FLAGS) $(W_FLAGS)



## Testcases

# Most test cases should output a libcrunch summary in which
# -- zero or more checks passed (trivially or by lazy typing)
# -- zero checks aborted
# -- zero checks failed
checkrun-%: %/check-stamp
	true # remembered pass of $*
%/check-stamp: $(RERUN_TESTS_ON_CHANGE)
	@echo "case is $*" && \
	out=$$( echo -n "out is " 1>&2; mktemp | tee /dev/stderr ) && \
	err=$$( echo -n "err is " 1>&2; mktemp | tee /dev/stderr ) && \
	$(MAKE) cleanrun-$* 1>$$out 2>$$err && \
		. ./checks.sh && tail -n32 "$$err" | \
		    match "       nontrivially passed" '[1-9][0-9]*' | \
		    match "       failed inside"        '0' | \
		    match "       failed otherwise"     '0' >/dev/null && \
		    touch "$@" || \
		(tail -n32 $$err; false)

checkrun-my-test: my-test/check-stamp
my-test/check-stamp: $(RERUN_TESTS_ON_CHANGE)
	$(MAKE) cleanrun-my-test >/dev/null 2>&1 && touch "$@"

checkrun-index-tree-test: index-tree-test/check-stamp
index-tree-test/check-stamp: $(RERUN_TESTS_ON_CHANGE)
	$(MAKE) cleanrun-index-tree-test >/dev/null 2>&1 && touch "$@"

checkrun-hello-errno: hello-errno/check-stamp
hello-errno/check-stamp: $(RERUN_TESTS_ON_CHANGE)
	$(MAKE) cleanrun-hello-errno >/dev/null 2>&1 && touch "$@"


## All of our parameterised rules for each test case

_onlyrun-%:
	./$*

_onlygdbrun-%:
	gdb --args ./$*

# Rule which can be run from test dirs to build the exec
_build-%: $(LIBCRUNCH_OBJS)
	# crunchcc the target
	$(CRUNCHCC) $*.c -c -o $*.o $(DEBUG_FLAGS) $(W_FLAGS) -include ../testsheader.h -DIS_INTERNAL_TEST=$(IS_INTERNAL_TEST)
	# link without types
	$(PLAIN_CC) $(LIBCRUNCH_OBJS) $*.o -o $* $(WRAP_FLAGS) -lc $(DEBUG_FLAGS) -Wl,--Map,/tmp/ldmap1
	# debug
	# cp $* $*.without_types
	# dump the types of that
	$(DUMPTYPES) $* > types.c 2> /dev/null
	$(TYPESSPLITTER) types.c 1
	# then build types
	$(PLAIN_CC) types1.c -c -o types1.o -I.
	$(PLAIN_CC) typesstack.c -c -o typesstack.o -I.
	objcopy --weaken types1.o
	objcopy --weaken typesstack.o
	# then link with types
	$(PLAIN_CC) $(LIBCRUNCH_OBJS) types1.o typesstack.o $*.o -o $* $(WRAP_FLAGS) -lc $(DEBUG_FLAGS) -Wl,--Map,/tmp/ldmap2

build-%:
	$(MAKE) -C "$*" $(INCLUDE_MK_INC) -f ../Makefile _build-$*

run-%:
	$(MAKE) build-$* && ( $(MAKE) -C "$*" $(INCLUDE_MK_INC) -f ../Makefile _onlyrun-$* )

cleanbuild-%:
	$(MAKE) -C $* $(INCLUDE_MK_INC) -f ../Makefile clean && \
	$(MAKE) build-$*

cleanrun-%:
	$(MAKE) -C $* $(INCLUDE_MK_INC) -f ../Makefile clean && \
	$(MAKE) run-$*

gdbrun-%: # run the test case with itself as input
	$(MAKE) build-$* && \
	( cd "$*" && $(MAKE) $(INCLUDE_MK_INC) -f ../Makefile _onlygdbrun-$* )

gdbcleanrun-%:
	$(MAKE) -C $* $(INCLUDE_MK_INC) -f ../Makefile clean && $(MAKE) gdbrun-$*

clean-%:
	$(MAKE) -C "$*" $(INCLUDE_MK_INC) -f $(realpath $(THIS_MAKEFILE)) clean

# generic clean rule that we can run from test dirs too (with $(MAKE) -f ../Makefile)
clean: clear-stamps
	# (delete anything whose name is a prefix of a .c file's and doesn't contain a dot)
	rm -f $(filter-out .,$(patsubst %.c,%,$(shell find . -name '*.c')))
	rm -f $(filter-out .,$(patsubst %.cc,%,$(shell find . -name '*.cc')))
	find . -name 'types*.*' -o -name '*.cil.*'  -o -name '*.i' -o -name '*.o' \
		-o -name '*.s' -o -name '*.allocs' -o -name '*.without_types' | xargs rm -f
