#!/bin/sh

dynamorioVersion="9.93.19566"
dynamorioDir="DynamoRIO-Linux-${dynamorioVersion}"
dynamorioURL="https://github.com/DynamoRIO/dynamorio/releases/download/cronbuild-${dynamorioVersion}/${dynamorioDir}.tar.gz"

libdwarfVersion="0.7.0"
libdwarfDir="libdwarf-${libdwarfVersion}"
libdwarfURL="https://github.com/davea42/libdwarf-code/releases/download/v${libdwarfVersion}/${libdwarfDir}.tar.xz"

minUnitURL="https://raw.githubusercontent.com/siu/minunit/master/minunit.h"

drrun="external/${dynamorioDir}/bin64/drrun"
client="build/libjsontracer.so"
