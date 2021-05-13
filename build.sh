#!/bin/sh
# cmake_build_type=Debug
cmake_build_type=Release

rm build -rf
mkdir -p build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=${cmake_build_type}
make -j
cd ../
#docker build -f DockerFile -t audio_db ./
#docker run -p 10080:80 -d --restart=always --ulimit core=-1 -v /data/corefiles:/data/corefiles  audio_db  audio 80
