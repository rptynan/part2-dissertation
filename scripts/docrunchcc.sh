#!/usr/local/bin/bash

if [ -z "$1" ]; then
    echo "Usage: ./docrunchcc.sh <compiler> [<disable KERNFAST>] [<extra make flags>]"
    echo "<compiler> can be either gcc or crunchcc"
    echo "<disable KERNFAST> if this is remakeeverything, it will wipe all your"
    echo "build before rebuilding, as opposed to just building missing files."
fi

kernfast_option=""
[[ "$2" != remakeeverything ]] && kernfast_option="-DKERNFAST"

compile_with_crunchcc(){
    # DEBUG_CC=1 \
    CC='/usr/local/src/libcrunch/frontend/c/bin/crunchcc -gdwarf-3 -gdwarf-2 -D_MM_MALLOC_H_INCLUDED --useLogicalOperators' \
    COPTFLAGS='-O -pipe' \
    COMPILER_TYPE='gcc' \
    CFLAGS='-fpermissive' \
    CPPFLAGS='' CXXFLAGS='' LDFLAGS='' \
    WITHOUT_FORMAT_EXTENSIONS=no \
    /usr/bin/time \
    make buildkernel $kernfast_option $3 KERNCONF=CRUNCHED \
    2> ~/errorlog | tee ~/log
    # -d A \
    # ;
}

compile_with_gcc(){
    CC='/usr/local/bin/gcc -gdwarf-3 -gdwarf-2 -D_MM_MALLOC_H_INCLUDED' \
    COPTFLAGS='-O -pipe' \
    COMPILER_TYPE='gcc' \
    CFLAGS='-fpermissive' \
    CPPFLAGS='' CXXFLAGS='' LDFLAGS='' \
    WITHOUT_FORMAT_EXTENSIONS=no \
    /usr/bin/time \
    make buildkernel $kernfast_option $3 KERNCONF=CRUNCHED \
    2> ~/errorlog | tee ~/log
    # -d A \
    # ;
}

case $1 in
    gcc) compile_with_gcc;;
    crunch*) compile_with_crunchcc;;
esac
exit
