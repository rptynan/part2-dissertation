#!/usr/local/bin/bash

function get_symbol_for_addr() {
	gdb -batch -ex "file $CRUNCHED/kernel.debug" -ex "info symbol 0x$1"
}

function parse_one_failure() {
	obj_addr=$(cat $1 | grep "__is_a" \
				| sed 's/.*__is_a(0x\([a-f0-9]*\),.*/\1/')
	test_type_addr=$(cat $1 | grep "__is_a" \
				| sed 's/.*, 0x\([a-f0-9]*\)).*/\1/')

	alloc_type_addr=$(cat $1 | grep "alloc type of" \
				| sed 's/.*alloc type of 0x\([a-f0-9]*\).*/\1/')
	origin_addr=$(cat $1 | grep "originating at" \
				| sed 's/.*originating at 0x\([a-f0-9]*\).*/\1/')
	# echo $obj_addr
	# echo $test_type_addr
	# echo $alloc_type_addr
	# echo $origin_addr

	echo "Failed to check object: $(get_symbol_for_addr $obj_addr)"
	echo "\tas type: $(get_symbol_for_addr $test_type_addr)"
	echo "\toriginating at: $(get_symbol_for_addr $origin_addr)," \
		"$(addr2line -e $CRUNCHED/kernel.debug $origin_addr)"
	echo "\trecorded as type:  $(get_symbol_for_addr $alloc_type_addr)"
}

# Pipe input lines in order into this (note: ktrdump is backwards, use tac)
function parse_failures() {
	while read line; do
		case "$line" in
			*"__is_a("*)
				obj_addr=$(echo $line | grep "__is_a" \
					| sed 's/.*__is_a(0x\([a-f0-9]*\),.*/\1/')
				test_type_addr=$(echo $line | grep "__is_a" \
					| sed 's/.*, 0x\([a-f0-9]*\)).*/\1/')
				echo "Failed __is_a check object: $(get_symbol_for_addr $obj_addr)"
				echo "\tas type: $(get_symbol_for_addr $test_type_addr)"
				;;
			*"__like_a("*)
				obj_addr=$(echo $line | grep "__like_a" \
					| sed 's/.*__like_a(0x\([a-f0-9]*\),.*/\1/')
				test_type_addr=$(echo $line | grep "__like_a" \
					| sed 's/.*, 0x\([a-f0-9]*\)).*/\1/')
				echo "Failed __like_a check object: $(get_symbol_for_addr $obj_addr)"
				echo "\tas type: $(get_symbol_for_addr $test_type_addr)"
				;;
			*"alloc type of"*)
				alloc_type_addr=$(echo $line | grep "alloc type of" \
					| sed 's/.*alloc type of 0x\([a-f0-9]*\).*/\1/')
				echo "\trecorded as type:  $(get_symbol_for_addr $alloc_type_addr)"
				;;
			*"originating at"*)
				origin_addr=$(echo $line | grep "originating at" \
					| sed 's/.*originating at 0x\([a-f0-9]*\).*/\1/')
				echo "\toriginating at: $(get_symbol_for_addr $origin_addr)," \
					"$(addr2line -e $CRUNCHED/kernel.debug $origin_addr)"
				;;
			*) ;;
		esac
	done
	return
}

function dump_failures() {
	ktrdump | tac | parse_failures
}

function ktr_on() {
	sysctl debug.ktr.mask=`sysctl -n debug.ktr.compile`
}

function ktr_off() {
	sysctl debug.ktr.mask=0
}
