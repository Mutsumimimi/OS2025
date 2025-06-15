#! /bin/bash

printf "Usage: bash ./run.sh <--first-fit|--best-fit> [--debug]\n"


fitmode=$1
debug="DEBUG=-UDEBUG"

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --first-fit) fitmode=1; shift ;;
        --best-fit) fitmode=0; shift ;;
        --debug) debug="DEBUG=-DDEBUG"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
done

TRACEPATH=$(case "$0" in /*) echo "`dirname $0`" ;; *) echo "$(pwd)/`dirname $0`" ;; esac)
MALLOCPATH="$TRACEPATH/../malloclab/"
export LD_LIBRARY_PATH=$MALLOCPATH:$LD_LIBRARY_PATH
cd $MALLOCPATH; make clean
make FIRST_FIT=$fitmode $debug
cd $TRACEPATH
g++ -g workload.cc -o workload -I$MALLOCPATH -L$MALLOCPATH -lmem -lpthread -std=c++11
./workload
