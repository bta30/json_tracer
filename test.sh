#!/bin/sh

./build.sh
cd build
ctest --rerun-failed --output-on-failure
cd ..
rm -f *.log
