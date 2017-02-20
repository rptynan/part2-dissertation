#!/usr/bin/local/bash

# Tools for dealing with linking kernel just for a certain set of files,
# going by first letters or by a list of filenames.
#
# Setup: be in /usr/obj/usr/src/sys/, with build going in CRUNCHED, and all the
# .o files from a gcc build in CRUNCHED_gcc and similarly for crunchcc.

# cplettersin <letters> <from gcc or crunchcc>
function cplettersin() {
    for i in {$1}; do
        cp -f CRUNCHED_$2/$i*.o CRUNCHED
    done
}

# cplettersin <letters> <to gcc or crunchcc>
function cplettersout() {
    for i in {$1}; do
        cp -f CRUNCHED/$i*.o CRUNCHED_$2/
    done
}

# checkletter <letter>
function checkletter() {
    inCRUNCHED=0
    for i in $(find CRUNCHED -name "$1*.o"); do
        [[ $(readelf -s $i | grep libcrunch) ]] && inCRUNCHED=$(($inCRUNCHED+1))
    done
    echo "In CRUNCHED there are $inCRUNCHED crunched files"
    echo "Out of $(ls CRUNCHED/$1*.o | wc -l) files"

    ingcc=0
    for i in $(find CRUNCHED_gcc -name "$1*.o"); do
        [[ $(readelf -s $i | grep libcrunch) ]] && ingcc=$(($ingcc+1))
    done
    echo "In gcc files there are $ingcc crunched files"
    echo "Out of $(ls CRUNCHED_gcc/$1*.o | wc -l) files"

    incrunchcc=0
    for i in $(find CRUNCHED_crunchcc -name "$1*.o"); do
        [[ $(readelf -s $i | grep libcrunch) ]] && incrunchcc=$(($incrunchcc+1))
    done
    echo "In crunchcc files there are $incrunchcc crunched files"
    echo "Out of $(ls CRUNCHED_crunchcc/$1*.o | wc -l) files"
}

# populate gcc=<letters with gcc> crunchcc=<letters with crunchcc>
function populatedir() {
    gccletters=$(echo $1 | sed 's/gcc=//g')
    echo "Following letters will be from gcc: $gccletters"
    for i in {$gccletters}; do
        cplettersin $i gcc
    done

    crunchccletters=$(echo $2 | sed 's/crunchcc=//g')
    echo "Following letters will be from crunchcc: $crunchccletters"
    for i in {$crunchccletters}; do
        cplettersin $i crunchcc
    done

    echo "Sanity check, there's $(ls CRUNCHED/*.o | wc -l) .o files in CRUNCHED"
}

# populatefromfile <file>
# Populate according to file specified as so:
# filenameA.o gcc
# filenameB.o crunchcc
function populatefromfile() {
    while read line; do
        compiler=$(echo $line | cut -d ' ' -f 2)
        filename=$(echo $line | cut -d ' ' -f 1)
        echo "cp -f CRUNCHED_$compiler/$filename CRUNCHED/"
        cp -f CRUNCHED_$compiler/$filename CRUNCHED/
    done < $1
}
