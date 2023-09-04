#!/bin/bash

cd $(dirname $0)

function buildTests {
    rm -rf build_test/ &&
    mkdir build_test &&
    cd build_test &&
    cmake ../test &&
    make &&
    cd ..
}

function runTests {
    rm -f *.log &&
    cd build_test &&
    ctest --rerun-failed --output_on_failure &&
    cd .. &&
    rm -f *.log
}

./build.sh &&
buildTests &&
runTests
