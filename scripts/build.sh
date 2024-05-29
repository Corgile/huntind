#!/bin/sh

cmake -G Ninja \
-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_C_COMPILER=gcc-13 \
-DCMAKE_CXX_COMPILER=g++-13 \
-DCMAKE_LINKER=/usr/bin/lld \
-DHD_USE_THREADS=ON \
-DHD_USE_JE_MALLOC=ON \
-DHD_ENABLE_LOGS=ON \
-DHD_LOG_LEVEL=INFO \
-DHD_USE_CUDA=ON \
-S .. -B .

cmake --build . -j64
