#!/bin/sh

rm -rf build-x-*
SUBFILENAME=$(date "+%Y%m%d-%H%M%S")
mkdir "build-x-$SUBFILENAME"
cd build-x-$SUBFILENAME || exit
cmake -DCMAKE_BUILD_TYPE=Release ../..
cmake --build . --config Release -j 4
./process_test

