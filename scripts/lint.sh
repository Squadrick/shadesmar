#!/bin/sh

cpplint --filter=-build/c++11,-build/include_subdir --exclude="include/catch.hpp" --exclude="build/*" --exclude="cmake-build-debug_ninja" --exclude="cmake-build-release_ninja" --recursive .
