#!/bin/sh

cd $(dirname $0)
source ./config.sh

function getDynamorio {
    curl -L ${dynamorioURL} -s | tar -xz -C external/
}

function getLibdwarf {
    curl -L ${libdwarfURL} -s | tar -xJ -C external/
    cd external/${libdwarfDir}
    mkdir build
    cd build
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE=True ..
    make
    cd ../../..
}

function getMinUnit {
    curl -L ${minUnitURL} >test/include/minunit.h
}

function getDeps {
    mkdir external/
    getDynamorio
    getLibdwarf
    getMinUnit
    build
}

function getBuild {
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

[ -d external/ ] || getDeps
[ -d build/ ] || getBuild
build
