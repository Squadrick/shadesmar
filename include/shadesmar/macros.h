/* MIT License

Copyright (c) 2020 Dheeraj R Reddy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

#ifndef INCLUDE_SHADESMAR_MACROS_H_
#define INCLUDE_SHADESMAR_MACROS_H_

#include <chrono>
#include <iostream>

#define TIMESCALE std::chrono::microseconds
#define TIMESCALE_COUNT 1e6
#define TIMESCALE_NAME "us"

#ifdef DEBUG_BUILD
#define DEBUG_IMPL(str, eol) \
  do {                       \
    std::cout << str << eol; \
  } while (0);

#define TIMEIT(cmd, name)                                                   \
  do {                                                                      \
    auto start = std::chrono::system_clock::now();                          \
    cmd;                                                                    \
    auto end = std::chrono::system_clock::now();                            \
    auto diff = std::chrono::duration_cast<TIMESCALE>(end - start).count(); \
    DEBUG("Time for " << name << ": " << diff << TIMESCALE_NAME);           \
  } while (0);

#else

#define DEBUG_IMPL(str, eol) \
  do {                       \
  } while (0);

#define TIMEIT(cmd, name) cmd;
#endif

#define DEBUG(str) DEBUG_IMPL(str, "\n");
#define INFO_INIT 1337

const int MAX_SHARED_OWNERS = 4;

#endif  // INCLUDE_SHADESMAR_MACROS_H_
