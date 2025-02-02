#!/bin/bash

mkdir -p build
cd build
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DOJPH_DISABLE_SIMD=ON && emmake make -j8 && mv libopenjph.* ../html/
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DOJPH_DISABLE_SIMD=OFF && emmake make -j8 && mv libopenjph.wasm ../html/libopenjph_simd.wasm
cd ..
sed 's/libopenjph.wasm/libopenjph_simd.wasm/g' build/libopenjph.js > html/libopenjph_simd.js
rm build/libopenjph.js
