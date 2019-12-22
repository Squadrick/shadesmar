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

#define RPC_QUEUE_SIZE 32

namespace shm {
class Channel : public Memory<RPC_QUEUE_SIZE> {
public:
  Channel(const std::string &name, bool caller)
      : Memory<RPC_QUEUE_SIZE>(name), caller_(caller), idx_(0) {}

  bool write(void *data, size_t size);
  bool read(msgpack::object_handle &oh, uint32_t pos);

  bool _write_caller(void *data, size_t size);
  bool _write_server(void *data, size_t size);
  void _write(Element *elem, void *data, size_t size);

  int32_t idx_;

private:
  bool caller_;
};

bool Channel::write(void *data, size_t size) {
  if (size > this->raw_buf_->get_free_memory()) {
    std::cerr << "Increase max_buffer_size" << std::endl;
    return false;
  }

  if (caller_) {
    return _write_caller(data, size);
  } else {
    return _write_server(data, size);
  }
}

bool Channel::_write_caller(void *data, size_t size) {
  idx_ = this->fetch_add_counter();
  uint32_t pos = idx_ & (RPC_QUEUE_SIZE - 1);
  auto elem = &(this->shared_queue_->elements[pos]);

  bool exp = true;
  if (!elem->empty.compare_exchange_strong(exp, false)) {
    return false;
  }
  ScopeGuard _(&elem->mutex);

  _write(elem, data, size);
  elem->ready = false;

  return true;
}

bool Channel::_write_server(void *data, size_t size) {
  uint32_t pos = idx_ & (RPC_QUEUE_SIZE - 1);
  auto elem = &(this->shared_queue_->elements[pos]);

  ScopeGuard _(&elem->mutex);
  this->raw_buf_->deallocate(
      this->raw_buf_->get_address_from_handle(elem->addr_hdl));

  _write(elem, data, size);
  elem->ready = true;

  return true;
}

void Channel::_write(Element *elem, void *data, size_t size) {

  void *new_addr = this->raw_buf_->allocate(size);
  std::memcpy(new_addr, data, size);
  elem->addr_hdl = this->raw_buf_->get_handle_from_address(new_addr);
  elem->size = size;
}

bool Channel::read(msgpack::object_handle &oh, uint32_t pos) {
  pos &= RPC_QUEUE_SIZE - 1;
  auto elem = &(this->shared_queue_->elements[pos]);

  if (caller_) {
    bool exp = true;
    if (!elem->ready.compare_exchange_strong(exp, false)) {
      return false;
    }
  }
  //  DEBUG(caller_);

  ScopeGuard _(&elem->mutex);

  const char *dst = reinterpret_cast<const char *>(
      this->raw_buf_->get_address_from_handle(elem->addr_hdl));

  try {
    DEBUG("try unpack");
    oh = msgpack::unpack(dst, elem->size);
  } catch (...) {
    DEBUG("unpack fail");
    return false;
  }

  if (caller_) {
    elem->empty = true;
  }

  return true;
}

} // namespace shm

#endif // SHADESMAR_CHANNEL_H
