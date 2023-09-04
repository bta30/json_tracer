#!/bin/bash

cd $(dirname $0)

function buildTests {
    mkdir build_test
    cd build_test
    cmake ../test
    make
    cd ..
}

rm -f *.log
./build.sh
buildTests
cd build_test
ctest --rerun-failed --output-on-failure
cd ..
rm -f *.log
