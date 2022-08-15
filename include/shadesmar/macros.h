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

#include <sys/stat.h>

#include <chrono>
#include <iostream>
#include <string>

#define TIMESCALE std::chrono::microseconds
#define TIMESCALE_COUNT 1e6
#define TIMESCALE_NAME "us"

namespace shm {
uint64_t current_time() {
  auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
  auto casted_time = std::chrono::duration_cast<TIMESCALE>(time_since_epoch);
  return casted_time.count();
}
}  // namespace shm

inline bool proc_dead(__pid_t proc) {
  if (proc == 0) {
    return false;
  }
  std::string pid_path = "/proc/" + std::to_string(proc);
  struct stat sts {};
  return (stat(pid_path.c_str(), &sts) == -1 && errno == ENOENT);
}

#endif  // INCLUDE_SHADESMAR_MACROS_H_
