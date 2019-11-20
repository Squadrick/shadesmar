//
// Created by squadrick on 14/11/19.
//

#ifndef SHADESMAR_ALLOCATOR_H
#define SHADESMAR_ALLOCATOR_H

namespace shm {
struct MemoryBlock {
  uint8_t *addr;
  size_t size;
};

struct Info {
  uint8_t init;
  size_t rem_size;

  MemoryBlock *blocks;
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
  Allocator(uint8_t *raw_buffer, size_t);
  uint8_t *malloc(size_t size);
  void free(uint8_t *ptr);

  size_t get_offset(uint8_t *addr) const;
  uint8_t *set_offset(size_t offset) const;

private:
  uint8_t *raw_buffer_;
  size_t total_size_;
};

Allocator::Allocator(uint8_t *raw_buffer, size_t total_size)
    : raw_buffer_(raw_buffer), total_size_(total_size) {}

uint8_t *Allocator::malloc(size_t size) { return nullptr; }

void Allocator::free(uint8_t *ptr) { return; }

size_t Allocator::get_offset(uint8_t *addr) const { return addr - raw_buffer_; }

uint8_t *Allocator::set_offset(size_t offset) const {
  return raw_buffer_ + offset;
}
} // namespace shm
#endif // SHADESMAR_ALLOCATOR_H
