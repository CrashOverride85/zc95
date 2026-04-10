#!/bin/sh

mkdir -p /src/source/CompiledUF2

cd /src/source/zc95Bootloader
rm -f CMakeCache.txt 
mkdir -p build
cd build
rm -f CMakeCache.txt 
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
make

cd /src/source/zc95
rm -f CMakeCache.txt 
mkdir -p build
cd build
rm -f CMakeCache.txt 
cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
make -j8
cp zc95.uf2 /src/source/CompiledUF2/zc95.uf2

cd /src/source/zc624Bootloader
rm -f CMakeCache.txt 
mkdir -p build
cd build
rm -f CMakeCache.txt 
cmake ..
make -j8

cd /src/source/zc624
rm -f CMakeCache.txt 
mkdir -p build
cd build
rm -f CMakeCache.txt 
cmake ..
make -j8
cp OutputZc.uf2 /src/source/CompiledUF2/OutputZc.uf2

