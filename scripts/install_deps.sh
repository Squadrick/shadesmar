#!/bin/sh
# Google benchmark
git clone https://github.com/google/benchmark.git google_benchmark
cd google_benchmark
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
make -j 4
sudo make install
