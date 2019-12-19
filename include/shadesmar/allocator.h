//
// Created by squadrick on 14/11/19.
//

#ifndef SHADESMAR_ALLOCATOR_H
#define SHADESMAR_ALLOCATOR_H

#include <atomic>

namespace shm {
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
      total_size_(total_size), free_size_(total_size), elems_(elems), idx_(0) {
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

} // namespace shm
#endif // SHADESMAR_ALLOCATOR_H
