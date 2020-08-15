#!/bin/sh

cpplint \
  --filter=-build/c++11,-build/include_subdir,-build/include_order \
  --exclude="include/catch.hpp" \
  --exclude="build/*" \
  --exclude="cmake-build*" \
  --recursive .
