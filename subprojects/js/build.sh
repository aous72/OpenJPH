#!/bin/sh
mkdir -p build
#(cd build && emcmake cmake -DCMAKE_BUILD_TYPE=Debug ..)
(cd build && emcmake cmake ..)
(cd build && emmake make VERBOSE=1 -j)
