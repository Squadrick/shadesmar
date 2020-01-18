//
// Created by squadrick on 8/2/19.
//

#ifndef shadesmar_TOPIC_H
#define shadesmar_TOPIC_H

#include <cstring>

#include <atomic>
#include <iostream>
#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>

#include <shadesmar/macros.h>
#include <shadesmar/memory.h>

using namespace boost::interprocess;
namespace shm {

template <uint32_t queue_size = 1> class Topic : public Memory<queue_size> {

  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

public:
  explicit Topic(const std::string &topic, bool extra_copy = false)
      : Memory<queue_size>(topic), extra_copy_(extra_copy) {}

  bool write(void *data, size_t size) {
    if (size > this->raw_buf_->get_free_memory()) {
      std::cerr << "Increase max_buffer_size" << std::endl;
      return false;
    }
    uint32_t pos =
        this->fetch_add_counter() & (queue_size - 1); // mod queue_size
    auto elem = &(this->shared_queue_->elements[pos]);
    return _write_rcu(data, size, elem);
  }

  bool _write(void *data, size_t size, Element *elem) {
    ScopeGuard<RobustLock> _(&(elem->mutex));

    void *addr = this->raw_buf_->get_address_from_handle(elem->addr_hdl);

    if (!elem->empty) {
      this->raw_buf_->deallocate(addr);
    }

    void *new_addr = this->raw_buf_->allocate(size);

    std::memcpy(new_addr, data, size);
    elem->addr_hdl = this->raw_buf_->get_handle_from_address(new_addr);
    elem->size = size;
    elem->empty = false;

    return true;
  }

  bool _write_rcu(void *data, size_t size, Element *elem) {
    void *new_addr = this->raw_buf_->allocate(size);
    std::memcpy(new_addr, data, size);

    ScopeGuard<RobustLock> _(&(elem->mutex));

    void *addr = this->raw_buf_->get_address_from_handle(elem->addr_hdl);
    bool prev_empty = elem->empty;
    elem->addr_hdl = this->raw_buf_->get_handle_from_address(new_addr);
    elem->size = size;
    elem->empty = false;

    if (!prev_empty) {
      this->raw_buf_->deallocate(addr);
    }

    return true;
  }

  bool read(msgpack::object_handle &oh, uint32_t pos) {
    pos &= queue_size - 1;
    auto elem = &(this->shared_queue_->elements[pos]);

    if (elem->empty) {
      return false;
    }

    if (extra_copy_)
      return _read_with_copy(oh, elem);
    else
      return _read_without_copy(oh, elem);
  }

  bool read(std::unique_ptr<uint8_t[]> &msg, size_t &size, uint32_t pos) {
    auto elem = &(this->shared_queue_->elements[pos & (queue_size - 1)]);
    return _read_bin(msg, size, elem);
  }

  bool _read_without_copy(msgpack::object_handle &oh, Element *elem) {
    ScopeGuard<RobustLock> _(&elem->mutex, ScopeGuard<RobustLock>::SHARED);

    const char *dst = reinterpret_cast<const char *>(
        this->raw_buf_->get_address_from_handle(elem->addr_hdl));

    try {
      oh = msgpack::unpack(dst, elem->size);
    } catch (...) {
      return false;
    }

    return true;
  }

  bool _read_with_copy(msgpack::object_handle &oh, Element *elem) {
    auto src = std::unique_ptr<uint8_t[]>(new uint8_t[elem->size]);
    size_t size;

    if (!_read_bin(src, size, elem)) {
      return false;
    }

    try {
      oh = msgpack::unpack(reinterpret_cast<const char *>(src.get()), size);
    } catch (...) {
      return false;
    }

    return true;
  }

  bool _read_bin(std::unique_ptr<uint8_t[]> &msg, size_t &size, Element *elem) {
    ScopeGuard<RobustLock> _(&(elem->mutex), ScopeGuard<RobustLock>::SHARED);

    if (elem->empty)
      return false;

    size = elem->size;
    msg = std::unique_ptr<uint8_t[]>(new uint8_t[elem->size]);
    auto *dst = this->raw_buf_->get_address_from_handle(elem->addr_hdl);
    std::memcpy(msg.get(), dst, elem->size);

    return true;
  }

private:
  bool extra_copy_;
};
} // namespace shm
#endif // shadesmar_TOPIC_H
