#!/usr/local/bin/bash

# Used for debugging static vars offsets

function get_actual_addr() {
	objdump -tT $CRUNCHED/kernel.debug | grep $1 | head -n 1 \
		| tr -s "[:blank:]" " " \
		| cut -d " " -f 1
}

function get_apparent_addr() {
	grep $1  /usr/src/sys/libcrunchk/types/typesstack.c | head -n 1 \
		| sed 's;.*at vaddr 0x\([a-f0-9]*\) \*\/;\1;'
}

function get_section() {
	objdump -tT $CRUNCHED/kernel.debug | grep $1 | head -n 1 \
		| tr -s "[:blank:]" " " \
		| cut -d " " -f 4
}

function hex_addr_diff() {
	python -c "print(hex(int('$1', 16) - int('$2', 16)))" | sed 's/L$//'
}

function symbol_addr_diff() {
	if [ -z $1 ] || [ -z $2 ]; then
		echo "Usage: get_offsets <magic_symbol> <query_symbol>"
		return
	fi

	magic_symbol="$1"
	query_symbol="$2"

	echo "Magic symbol, section $(get_section $magic_symbol)"
	mact="0x$(get_actual_addr $magic_symbol)"
	echo "actual: $mact"
	mapp="0x$(get_apparent_addr $magic_symbol)"
	echo "apparent: $mapp"
	echo "offset: $(hex_addr_diff $mact $mapp)"

	echo "Query symbol, section $(get_section $query_symbol)"
	qact="0x$(get_actual_addr $query_symbol)"
	echo "actual: $qact"
	qapp="0x$(get_apparent_addr $query_symbol)"
	echo "apparent: $qapp"
	echo "offset: $(hex_addr_diff $qact $qapp)"
}
