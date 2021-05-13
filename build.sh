#!/bin/bash
rm -rf build ; mkdir -p build 
cd build 
cmake ../  -DCMAKE_BUILD_TYPE=Release
make -j
