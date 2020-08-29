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

#ifndef INCLUDE_SHADESMAR_MEMORY_ALLOCATOR_H_
#define INCLUDE_SHADESMAR_MEMORY_ALLOCATOR_H_

#include <cassert>

#include "shadesmar/concurrency/robust_lock.h"
#include "shadesmar/concurrency/scope.h"

namespace shm::memory {

class Allocator {
 public:
  using handle = uint64_t;
  template <concurrent::ExlOrShr type>
  using Scope = concurrent::ScopeGuard<concurrent::RobustLock, type>;

  Allocator(size_t offset, size_t size);
  uint8_t *alloc(uint32_t bytes);
  bool free(const uint8_t *ptr);
  void reset();

  inline handle ptr_to_handle(uint8_t *p) {
    return p - reinterpret_cast<uint8_t *>(heap_());
  }
  uint8_t *handle_to_ptr(handle h) {
    return reinterpret_cast<uint8_t *>(heap_()) + h;
  }

  size_t get_free_memory() {
    // TODO(squadrick): Implement this properly
    Scope<concurrent::EXCLUSIVE> _(&lock_);
    return size_;
  }

 private:
  void validate_index(uint32_t index) const;
  [[nodiscard]] uint32_t suggest_index(uint32_t header_index,
                                       uint32_t payload_size) const;
  uint32_t *__attribute__((always_inline)) heap_() {
    return reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(this) +
                                        offset_);
  }

  concurrent::RobustLock lock_;

  uint32_t alloc_index_;
  volatile uint32_t free_index_;
  size_t offset_;
  size_t size_;
};

Allocator::Allocator(size_t offset, size_t size)
    : alloc_index_(0), free_index_(0), offset_(offset), size_(size) {
  assert(!(size & (sizeof(int) - 1)));
}

void Allocator::validate_index(uint32_t index) const {
  assert(index < (size_ / sizeof(int)));
}

uint32_t Allocator::suggest_index(uint32_t header_index,
                                  uint32_t payload_size) const {
  validate_index(header_index);

  int32_t payload_index = header_index + 1;

  if (payload_index + payload_size - 1 >= size_ / sizeof(int)) {
    payload_index = 0;
  }
  validate_index(payload_index);
  validate_index(payload_index + payload_size - 1);
  return payload_index;
}

uint8_t *Allocator::alloc(uint32_t bytes) {
  uint32_t payload_size = bytes;

  if (payload_size == 0) {
    payload_size = sizeof(int);
  }

  if (payload_size >= size_ - 2 * sizeof(int)) {
    return nullptr;
  }

  if (payload_size & (sizeof(int) - 1)) {
    payload_size &= ~(sizeof(int) - 1);
    payload_size += sizeof(int);
  }

  payload_size /= sizeof(int);

  Scope<concurrent::EXCLUSIVE> _(&lock_);

  const auto payload_index = suggest_index(alloc_index_, payload_size);
  const auto free_index_th = free_index_;
  uint32_t new_alloc_index = payload_index + payload_size;

  if (alloc_index_ < free_index_th && payload_index == 0) {
    return nullptr;
  }

  if (payload_index <= free_index_th && free_index_th <= new_alloc_index) {
    return nullptr;
  }

  if (new_alloc_index == size_ / sizeof(int)) {
    new_alloc_index = 0;
    if (new_alloc_index == free_index_th) {
      return nullptr;
    }
  }

  assert(new_alloc_index != alloc_index_);
  validate_index(new_alloc_index);

  heap_()[alloc_index_] = payload_size;
  alloc_index_ = new_alloc_index;

  return reinterpret_cast<uint8_t *>(heap_() + payload_index);
}

bool Allocator::free(const uint8_t *ptr) {
  if (ptr == nullptr) {
    return true;
  }
  auto *heap = reinterpret_cast<uint8_t *>(heap_());

  Scope<concurrent::EXCLUSIVE> _(&lock_);

  assert(ptr >= heap);
  assert(ptr < heap + size_);
  validate_index(free_index_);

  uint32_t payload_size = heap_()[free_index_];
  uint32_t payload_index = suggest_index(free_index_, payload_size);

  if (ptr != reinterpret_cast<uint8_t *>(heap_() + payload_index)) {
    return false;
  }

  uint32_t new_free_index = payload_index + payload_size;
  if (new_free_index == size_ / sizeof(int)) {
    new_free_index = 0;
  }
  free_index_ = new_free_index;
  return true;
}

void Allocator::reset() {
  alloc_index_ = 0;
  free_index_ = 0;
}

}  // namespace shm::memory
#endif  // INCLUDE_SHADESMAR_MEMORY_ALLOCATOR_H_
