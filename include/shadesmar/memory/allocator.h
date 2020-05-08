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

#include <atomic>

namespace shm::memory {
struct MemEntry {
  size_t offset;
  size_t size;
  std::atomic<bool> free;

  MemEntry() : offset(0), size(0), free(true) {}
};

/*
 * Assume that the underlying shared memory needs to be broken
 * into roughly equal sized segments. We allocate an array at the
 * start of the memory block for keeping track of block information.
 * Every time we allocate any memory, we add a new Node to our list.
 * We align to 16 bytes for the underlying memory space.
 */
class Allocator {
 public:
  Allocator(uint8_t *, size_t, uint32_t);
  uint8_t *malloc(size_t size);
  void free(uint8_t *ptr);

  size_t get_offset(uint8_t *addr) const;
  uint8_t *set_offset(size_t offset) const;

 private:
  uint8_t *start_ptr_;
  uint8_t *buffer_ptr_;
  size_t total_size_, free_size_;
  uint32_t elems_;
  std::atomic<uint32_t> idx_;
  MemEntry *table;
};

Allocator::Allocator(uint8_t *raw_buffer, size_t total_size, uint32_t elems)
    : start_ptr_(raw_buffer),
      buffer_ptr_(raw_buffer + elems * sizeof(MemEntry)),
      total_size_(total_size),
      free_size_(total_size),
      elems_(elems),
      idx_(0) {
  table = new (start_ptr_) MemEntry[elems];
}

uint8_t *Allocator::malloc(size_t size) {
  if (size > free_size_) {
    return nullptr;
  }

  int table_idx = idx_ & (elems_ - 1);
  MemEntry *entry = &table[table_idx];
  if (entry->free.load()) {
    entry->free = false;
    idx_.fetch_add(1);
  } else {
  }

  return nullptr;
}

void Allocator::free(uint8_t *ptr) { return; }

}  // namespace shm::memory
#endif  // INCLUDE_SHADESMAR_MEMORY_ALLOCATOR_H_
