#!/bin/bash

function listoids() {
	sysctl -a debug.liballocs debug.libcrunch | grep -v splaying | cut -d ":" -f 1
}

function clearoids() {
	for oid in $(listoids); do
		sysctl $oid=0
	done
	echo "oids cleared!"
}

# takes test command as input
function dotest() {
	clearoids
	eval $1
	sysctl -a debug.liballocs debug.libcrunch
}

size=32G
function dofiletest() {
	dotest "sysbench --test=fileio --init-rng=on --file-total-size=$size --file-test-mode=rndrw --max-time=180s run"
}

function docputest() {
	dotest "sysbench --test=cpu --cpu-max-prime=20000 --num-threads=10 run"
}

function dothreadstest() {
	dotest "sysbench --test=threads --num-threads=4 --thread-locks=2 --max-time=60s run"
}

function domutextest() {
	dotest "sysbench --test=mutex --num-threads=512 run"
}

function domemorytest() {
	dotest "sysbench --test=memory --memory-total-size=16G --num-threads=4 run"
}



USAGE="./test.sh <output_name> <num_iterations> <prepare or cleanup files>"

if [ -z $1 ]; then
	echo "You must specify a name for this test"
	echo $USAGE
	exit
fi

if [ -z $2 ]; then
	echo "You must specify the amount of iterations"
	echo $USAGE
	exit
fi

if [[ "$3" == "prepare" ]]; then
	echo "Preparing files for fileio test"
	sysbench --test=fileio --file-total-size=$size prepare
	exit
elif [[ "$3" == "cleanup" ]]; then
	echo "Cleaning up fileio test files"
	sysbench --test=fileio --file-total-size=$size cleanup
	exit
fi


timestamp=$(date +"%Y%m%d-%H%M")
output_dir="$1-$timestamp"

for i in $(seq 1 $2); do
	for t in "filetest" "cputest" "threadstest" "mutextest" "memorytest"; do
		mkdir -p $output_dir/$t
		echo "$t - run $i"
		eval "do$t 2>&1 1> $output_dir/$t/run$i"
	done
done
