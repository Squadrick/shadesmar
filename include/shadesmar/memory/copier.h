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

#ifndef INCLUDE_SHADESMAR_MEMORY_COPIER_H_
#define INCLUDE_SHADESMAR_MEMORY_COPIER_H_

#include <cstring>
#include <memory>

namespace shm::memory {
class Copier {
 public:
  virtual ~Copier() = default;
  // Each derived class of `Copier` needs to also add `PtrT` for compatibility
  // with`MTCopier`. See `DefaultCopier` for example.
  virtual void *alloc(size_t) = 0;
  virtual void dealloc(void *) = 0;
  virtual void shm_to_user(void *, void *, size_t) = 0;
  virtual void user_to_shm(void *, void *, size_t) = 0;
};

class DefaultCopier : public Copier {
 public:
  using PtrT = uint8_t;
  void *alloc(size_t size) override { return malloc(size); }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    std::memcpy(dst, src, size);
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    std::memcpy(dst, src, size);
  }
};

}  // namespace shm::memory

#endif  // INCLUDE_SHADESMAR_MEMORY_COPIER_H_
