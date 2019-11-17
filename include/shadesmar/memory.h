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
#include <boost/interprocess/mapped_region.hpp>

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
    // TODO: Has contention on sh_q_exists
    bool sh_q_exists = tmp::exists(topic);
    if (!sh_q_exists)
      tmp::write_topic(topic);

    int fd;
    auto *base_addr =
        create_memory_segment(topic, fd, sizeof(SharedQueue<queue_size>));

    raw_buf_ = std::make_shared<managed_shared_memory>(
        open_or_create, (topic + "Raw").c_str(), max_buffer_size);

    if (!sh_q_exists) {
      sh_q_ = new (base_addr) SharedQueue<queue_size>();
      init_info();
    } else {
      sh_q_ = reinterpret_cast<SharedQueue<queue_size> *>(base_addr);
    }
  }

  ~Memory() = default;

  void init_info() {
    sh_q_->info_mutex.lock();
    if (sh_q_->init != INFO_INIT) {
      sh_q_->init = INFO_INIT;
      sh_q_->counter = 0;
    }
    sh_q_->info_mutex.unlock();
  }

  bool write(void *data, uint32_t pos, size_t size) {
    pos &= queue_size - 1; // modulo for power of 2
    sh_q_->lock(pos);

    if (size > raw_buf_->get_free_memory()) {
      std::cout << "Increase max_buffer_size" << std::endl;
      exit(0);
    }
    auto idx = &(sh_q_->__array[pos]);
    void *addr = raw_buf_->get_address_from_handle(idx->addr_hdl);

    if (!idx->empty) {
      raw_buf_->deallocate(addr);
    }

    void *new_addr = raw_buf_->allocate(size);

    std::memcpy(new_addr, data, size);
    idx->addr_hdl = raw_buf_->get_handle_from_address(new_addr);
    idx->size = size;
    idx->empty = false;

    sh_q_->unlock(pos);
    return true;
  }

  bool read_non_copy(msgpack::object_handle &oh, uint32_t pos) {
    // We deserialize directly from shared memory to local memory
    // Faster for large messages
    pos &= queue_size - 1;

    sh_q_->lock_sharable(pos);

    auto idx = &(sh_q_->__array[pos]);
    if (idx->empty)
      return false;

    const char *dst = reinterpret_cast<const char *>(
        raw_buf_->get_address_from_handle(idx->addr_hdl));
    try {
      oh = msgpack::unpack(dst, idx->size);
    } catch (...) {
      return false;
    }

    sh_q_->unlock_sharable(pos);
    return true;
  }

  bool read_with_copy(void **src, uint32_t &size, uint32_t pos) {
    // We copy from shared memory to local memory, and then deserialize it
    // Faster for small messages
    pos &= queue_size - 1;

    sh_q_->lock_sharable(pos);

    auto idx = &(sh_q_->__array[pos]);
    if (idx->empty)
      return false;

    void *dst = raw_buf_->get_address_from_handle(idx->addr_hdl);
    *src = malloc(idx->size);
    std::memcpy(*src, dst, idx->size);
    size = idx->size;

    sh_q_->unlock_sharable(pos);

    return true;
  }

  inline __attribute__((always_inline)) uint32_t counter() {
    return sh_q_->counter.load();
  }

  inline __attribute__((always_inline)) uint32_t fetch_inc_counter() {
    return sh_q_->counter.fetch_add(1);
  }

private:
  std::string topic_;

  std::shared_ptr<mapped_region> region_;
  std::shared_ptr<managed_shared_memory> raw_buf_;

  SharedQueue<queue_size> *sh_q_;
};
} // namespace shm
#endif // shadesmar_MEMORY_H
