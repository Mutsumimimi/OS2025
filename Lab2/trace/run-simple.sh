#! /bin/bash

printf "Usage: bash ./run-simple.sh\n"

TRACEPATH="$PWD/`dirname $0`"
MALLOCPATH="$TRACEPATH/../malloclab-simple/"
export LD_LIBRARY_PATH=$MALLOCPATH:$LD_LIBRARY_PATH
cd $MALLOCPATH; make clean
make FIRST_FIT=$fitmode $debug
cd $TRACEPATH
g++ -g workload.cc -o workload -I$MALLOCPATH -L$MALLOCPATH -lmem -lpthread -std=c++11
./workload
