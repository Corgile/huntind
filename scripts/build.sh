#!/bin/sh

cmake -G Ninja \
-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
-DCMAKE_BUILD_TYPE=Release \
-DUSE_JE_MALLOC=ON \
-DHD_ENABLE_LOGS=ON \
-DHD_LOG_LEVEL=INFO \
-DCMAKE_C_COMPILER=gcc-13 \
-DCMAKE_CXX_COMPILER=g++-13 \
-DCMAKE_LINKER=/usr/bin/lld \
-DUSE_THREADS=ON \
-DUSE_CUDA=ON \
-S .. -B .

cmake --build . -j64
