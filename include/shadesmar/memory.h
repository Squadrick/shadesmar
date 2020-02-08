//
// Created by squadrick on 27/11/19.
//

#ifndef shadesmar_MEMORY_H
#define shadesmar_MEMORY_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdint>

#include <atomic>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <msgpack.hpp>

#include <shadesmar/allocator.h>
#include <shadesmar/dragons.h>
#include <shadesmar/macros.h>
#include <shadesmar/memory.h>
#include <shadesmar/robust_lock.h>
#include <shadesmar/tmp.h>

using namespace boost::interprocess;

namespace shm {

uint8_t *create_memory_segment(const std::string &name, size_t size,
                               bool &new_segment) {
  int fd;
  while (true) {
    new_segment = true;
    fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd >= 0) {
      fchmod(fd, 0644);
    }
    if (errno == EEXIST) {
      fd = shm_open(name.c_str(), O_RDWR, 0644);
      if (fd < 0 && errno == ENOENT) {
        // the memory segment was deleted in the mean time
        continue;
      }
      new_segment = false;
    }
    break;
  }
  int result = ftruncate(fd, size);
  if (result == EINVAL) {
    return nullptr;
  }
  auto *ptr = static_cast<uint8_t *>(
      mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

  return ptr;
}

struct Element {
  size_t size;
  std::atomic<bool> empty = {true};
  std::atomic<bool> ready = {false}; // used for RPC
  RobustLock mutex = {};
  managed_shared_memory::handle_t addr_hdl;

  Element() {
    addr_hdl = 0;
    size = 0;
  }
};

template <uint32_t queue_size> class SharedQueue {
public:
  std::atomic_uint32_t init{}, counter{0};
  std::array<Element, queue_size> elements;
  RobustLock info_mutex;

  void lock(uint32_t idx) { elements[idx].mutex.lock(); }
  void unlock(uint32_t idx) { elements[idx].mutex.unlock(); }
  void lock_sharable(uint32_t idx) { elements[idx].mutex.lock_sharable(); }
  void unlock_sharable(uint32_t idx) { elements[idx].mutex.unlock_sharable(); }
};

static size_t max_buffer_size = (1U << 28);

template <uint32_t queue_size = 1> class Memory {
  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

public:
  explicit Memory(const std::string &name) : name_(name) {
    bool new_segment;
    auto *base_addr = create_memory_segment(
        name, sizeof(SharedQueue<queue_size>), new_segment);

    if (base_addr == nullptr) {
      std::cerr << "Could not create shared memory buffer" << std::endl;
    }

    raw_buf_ = std::make_shared<managed_shared_memory>(
        open_or_create, (name + "Raw").c_str(), max_buffer_size);

    if (new_segment) {
      shared_queue_ = new (base_addr) SharedQueue<queue_size>();
      init_info();
    } else {
      shared_queue_ = reinterpret_cast<SharedQueue<queue_size> *>(base_addr);
      while (shared_queue_->init != INFO_INIT)
        ;
    }
  }

  ~Memory() = default;

  void init_info() {
    shared_queue_->info_mutex.lock();
    if (shared_queue_->init != INFO_INIT) {
      shared_queue_->init = INFO_INIT;
      shared_queue_->counter = 0;
      tmp::write(name_);
    }
    shared_queue_->info_mutex.unlock();
  }

  inline __attribute__((always_inline)) uint32_t counter() {
    return shared_queue_->counter.load();
  }

  inline __attribute__((always_inline)) uint32_t fetch_add_counter() {
    return shared_queue_->counter.fetch_add(1);
  }

  inline __attribute__((always_inline)) void inc_counter() {
    shared_queue_->counter.fetch_add(1);
  }

  std::string name_;
  std::shared_ptr<managed_shared_memory> raw_buf_;
  SharedQueue<queue_size> *shared_queue_;
};

} // namespace shm

#endif // shadesmar_MEMORY_H
