#!/bin/sh

clang-format -i --style=file include/**/*.h test/*.cpp benchmark/*.cpp
