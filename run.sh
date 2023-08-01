#!/bin/sh

dynamorioVersion="9.93.19566"
dynamorioDir="DynamoRIO-Linux-${dynamorioVersion}"
dynamorioURL="https://github.com/DynamoRIO/dynamorio/releases/download/cronbuild-${dynamorioVersion}/${dynamorioDir}.tar.gz"

libdwarfVersion="0.7.0"
libdwarfDir="libdwarf-${libdwarfVersion}"
libdwarfURL="https://github.com/davea42/libdwarf-code/releases/download/v${libdwarfVersion}/${libdwarfDir}.tar.xz"

drrun="external/${dynamorioDir}/bin64/drrun"
client="build/libjsontracer.so"

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

function getDeps {
    mkdir external/
    getDynamorio
    getLibdwarf
    build
}

function getBuild {
    mkdir build/
    cd build
    cmake ..
    make
    cd ..
}

cd $(dirname $0)
[ -d external/ ] || getDeps
[ -d build/ ] || getBuild
${drrun} -c ${client} $@
