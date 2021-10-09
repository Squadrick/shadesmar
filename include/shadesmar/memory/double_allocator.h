/* MIT License

Copyright (c) 2021 Dheeraj R Reddy

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

#ifndef INCLUDE_SHADESMAR_MEMORY_DOUBLE_ALLOCATOR_H_
#define INCLUDE_SHADESMAR_MEMORY_DOUBLE_ALLOCATOR_H_

#include "shadesmar/memory/allocator.h"

namespace shm::memory {

class DoubleAllocator {
 public:
  DoubleAllocator(size_t offset, size_t size)
      : req(offset, size / 2), resp(offset + size / 2, size / 2) {}
  Allocator req;
  Allocator resp;

  void reset() {
    req.reset();
    resp.reset();
  }

  void lock_reset() {
    req.lock_reset();
    resp.lock_reset();
  }
};

}  // namespace shm::memory

#endif  // INCLUDE_SHADESMAR_MEMORY_DOUBLE_ALLOCATOR_H_
