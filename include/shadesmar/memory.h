//
// Created by squadrick on 8/2/19.
//

#ifndef shadesmar_MEMORY_H
#define shadesmar_MEMORY_H

#include <cstdint>
#include <cstring>

#include <atomic>
#include <iostream>
#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>

#include <msgpack.hpp>

#include <shadesmar/ipc_lock.h>
#include <shadesmar/macros.h>
#include <shadesmar/tmp.h>

using namespace boost::interprocess;
namespace shm {

uint8_t *create_memory_segment(const std::string &topic, int &fd, size_t size) {
  while (true) {
    fd = shm_open(topic.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd >= 0) {
      fchmod(fd, 0644);
    }
    if (errno == EEXIST) {
      fd = shm_open(topic.c_str(), O_RDWR, 0644);
      if (fd < 0 && errno == ENOENT) {
        continue;
      }
    }
    break;
  }
  ftruncate(fd, size);
  auto *ptr = static_cast<uint8_t *>(
      mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

  return ptr;
}

template <uint32_t queue_size> struct SharedQueue {
  struct Element {
    managed_shared_memory::handle_t addr_hdl{};
    size_t size{};
    bool empty = true;
  };
  std::atomic_uint32_t init{}, counter{};

  Element __array[queue_size]{};
  IPC_Lock info_mutex;
  IPC_Lock queue_mutexes[queue_size];

  void lock(uint32_t idx) { queue_mutexes[idx].lock(); }

  void unlock(uint32_t idx) { queue_mutexes[idx].unlock(); }

  void lock_sharable(uint32_t idx) { queue_mutexes[idx].lock_sharable(); }

  void unlock_sharable(uint32_t idx) { queue_mutexes[idx].unlock_sharable(); }
};

template <uint32_t queue_size = 1> class Memory {
  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

public:
  explicit Memory(const std::string &topic, size_t max_buffer_size = (1U << 28))
      : topic_(topic) {
    // TODO: Has contention on shared_queue_exists
    bool shared_queue_exists = tmp::exists(topic);
    if (!shared_queue_exists)
      tmp::write_topic(topic);

    auto *base_addr =
        create_memory_segment(topic, fd_, sizeof(SharedQueue<queue_size>));

    raw_buf_ = std::make_shared<managed_shared_memory>(
        open_or_create, (topic + "Raw").c_str(), max_buffer_size);

    if (!shared_queue_exists) {
      shared_queue_ = new (base_addr) SharedQueue<queue_size>();
      init_info();
    } else {
      shared_queue_ = reinterpret_cast<SharedQueue<queue_size> *>(base_addr);
    }
  }

  ~Memory() = default;

  void init_info() {
    shared_queue_->info_mutex.lock();
    if (shared_queue_->init != INFO_INIT) {
      shared_queue_->init = INFO_INIT;
      shared_queue_->counter = 0;
    }
    shared_queue_->info_mutex.unlock();
  }

  bool write(void *data, uint32_t pos, size_t size) {
    pos &= queue_size - 1; // modulo for power of 2
    shared_queue_->lock(pos);

    if (size > raw_buf_->get_free_memory()) {
      std::cout << "Increase max_buffer_size" << std::endl;
      exit(0);
    }
    auto elem = &(shared_queue_->__array[pos]);
    void *addr = raw_buf_->get_address_from_handle(elem->addr_hdl);

    if (!elem->empty) {
      raw_buf_->deallocate(addr);
    }

    void *new_addr = raw_buf_->allocate(size);

    std::memcpy(new_addr, data, size);
    elem->addr_hdl = raw_buf_->get_handle_from_address(new_addr);
    elem->size = size;
    elem->empty = false;

    shared_queue_->unlock(pos);
    return true;
  }

  bool read_without_copy(msgpack::object_handle &oh, uint32_t pos) {
    // We deserialize directly from shared memory to local memory
    // Faster for large messages
    pos &= queue_size - 1;

    shared_queue_->lock_sharable(pos);

    auto elem = &(shared_queue_->__array[pos]);
    if (elem->empty)
      return false;

    const char *dst = reinterpret_cast<const char *>(
        raw_buf_->get_address_from_handle(elem->addr_hdl));
    try {
      oh = msgpack::unpack(dst, elem->size);
    } catch (...) {
      return false;
    }

    shared_queue_->unlock_sharable(pos);
    return true;
  }

  bool read_with_copy(void **src, uint32_t &size, uint32_t pos) {
    // We copy from shared memory to local memory, and then deserialize it
    // Faster for small messages
    pos &= queue_size - 1;

    shared_queue_->lock_sharable(pos);

    auto elem = &(shared_queue_->__array[pos]);
    if (elem->empty)
      return false;

    void *dst = raw_buf_->get_address_from_handle(elem->addr_hdl);
    *src = malloc(elem->size);
    std::memcpy(*src, dst, elem->size);
    size = elem->size;

    shared_queue_->unlock_sharable(pos);

    return true;
  }

  inline __attribute__((always_inline)) uint32_t counter() {
    return shared_queue_->counter.load();
  }

  inline __attribute__((always_inline)) uint32_t fetch_inc_counter() {
    return shared_queue_->counter.fetch_add(1);
  }

private:
  int fd_;
  std::string topic_;
  std::shared_ptr<managed_shared_memory> raw_buf_;
  SharedQueue<queue_size> *shared_queue_;
};
} // namespace shm
#endif // shadesmar_MEMORY_H
