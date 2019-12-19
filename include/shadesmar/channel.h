//
// Created by squadrick on 20/11/19.
//

#ifndef SHADESMAR_CHANNEL_H
#define SHADESMAR_CHANNEL_H

#include <cstdint>
#include <cstring>

#include <atomic>
#include <iostream>
#include <string>

#include <msgpack.hpp>

#include <shadesmar/memory.h>

namespace shm {
template <uint32_t queue_size = 32> class Channel : public Memory<queue_size> {
public:
  Channel(const std::string &name) : Memory<queue_size>(name) {}

  bool write(uint8_t *ptr, size_t size);
  bool read(msgpack::object_handle &oh, uint32_t pos);
};

template <uint32_t queue_size>
bool Channel<queue_size>::write(uint8_t *ptr, size_t size) {
  if (size > this->raw_buf_->get_free_memory()) {
    std::cerr << "Increase max_buffer_size" << std::endl;
    return false;
  }

  uint32_t pos = this->fetch_add_counter() & (queue_size - 1);

  this->shared_queue_->lock(pos);
  auto elem = &(this->shared_queue_->elements[pos]);
  if (!elem->empty) {
  }
}

template <uint32_t queue_size>
bool Channel<queue_size>::read(msgpack::object_handle &oh, uint32_t pos) {
  return false;
}

} // namespace shm

#endif // SHADESMAR_CHANNEL_H
