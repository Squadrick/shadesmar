//
// Created by squadrick on 14/11/19.
//

#ifndef SHADESMAR_ALLOCATOR_H
#define SHADESMAR_ALLOCATOR_H

namespace shm {
class Allocator {
public:
  Allocator(uint8_t *raw_buffer);
  uint8_t *malloc(size_t size);
  void free(uint8_t *ptr);

  size_t get_offset(uint8_t *addr);
  uint8_t *set_offset(size_t offset);

private:
  uint8_t *raw_buffer_;
};

Allocator::Allocator(const uint8_t *raw_buffer) : raw_buffer_(raw_buffer) {}

uint8_t *Allocator::malloc(size_t size) { return nullptr; }

void Allocator::free(uint8_t *ptr) { return; }

size_t Allocator::get_offset(uint8_t *addr) { return addr - raw_buffer_; }

uint8_t *Allocator::set_offset(size_t offset) { return raw_buffer_ + offset; }
} // namespace shm
#endif // SHADESMAR_ALLOCATOR_H
