#!/bin/bash

cd $(dirname $0)
source ./config.sh

function buildLibdwarf {
    cd external/${libdwarfDir}
    mkdir build
    cd build
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE=True ..
    make
    cd ../../..
}

function getBuildDir {
    mkdir build/
    cd build
    cmake ..
    cd ..
}

function build {
    cd build
    make
    cd ..
}

rm -rf external/${libdwarfDir}/build build/
buildLibdwarf
getBuildDir
build
