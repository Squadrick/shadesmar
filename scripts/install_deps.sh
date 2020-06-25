#!/bin/sh
case "$(uname -s)" in

   Darwin)
      brew update
      brew install msgpack ninja
     ;;

   Linux)
      sudo apt update
      sudo apt install libmsgpack-dev ninja-build
     ;;

   *)
     echo 'Not supported'
     exit 1
     ;;
esac

# Google benchmark
git clone https://github.com/google/benchmark.git google_benchmark
cd google_benchmark
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
make -j 4
sudo make install
