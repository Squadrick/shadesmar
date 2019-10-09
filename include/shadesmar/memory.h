//
// Created by squadrick on 8/2/19.
//

#ifndef SHADERMAR_MEMORY_H
#define SHADERMAR_MEMORY_H

#include <sys/ipc.h>
#include <sys/types.h>

#include <csignal>
#include <cstdint>
#include <cstring>

#include <atomic>
#include <iostream>
#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <shadesmar/ipc_lock.h>
#include <shadesmar/macros.h>
#include <shadesmar/tmp.h>

using namespace boost::interprocess;
namespace shm {

template <uint32_t queue_size>

struct SharedQueue {
  struct Element {
    managed_shared_memory::handle_t addr_hdl{};
    size_t size{};
    bool empty = true;
  };
  std::atomic<uint32_t> init{}, counter{};

  Element __array[queue_size]{};
  IPC_Lock info_mutex;
  IPC_Lock queue_mutexes[queue_size];

  void lock(uint32_t idx) {
    DEBUG("SharedQueue lock [" << idx << "]: trying");
    queue_mutexes[idx].lock();
    DEBUG("SharedQueue lock [" << idx << "]: success");
  }

  void unlock(uint32_t idx) {
    DEBUG("SharedQueue unlock [" << idx << "]: trying");
    queue_mutexes[idx].unlock();
    DEBUG("SharedQueue unlock [" << idx << "]: success");
  }

  void lock_sharable(uint32_t idx) {
    DEBUG("SharedQueue share lock [" << idx << "]: trying");
    queue_mutexes[idx].lock_sharable();
    DEBUG("SharedQueue share lock [" << idx << "]: success");
  }

  void unlock_sharable(uint32_t idx) {
    DEBUG("SharedQueue share unlock [" << idx << "]: trying");
    queue_mutexes[idx].unlock_sharable();
    DEBUG("SharedQueue share unlock [" << idx << "]: success");
  }
};

template <uint32_t queue_size = 1>
class Memory {
  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

 public:
  explicit Memory(const std::string &topic, size_t max_buffer_size = (1U << 21))
      : topic_(topic) {
    // TODO: Has contention on sh_q_exists
    bool sh_q_exists = tmp::exists(topic);
    if (!sh_q_exists) tmp::write_topic(topic);

    auto shm_obj =
        shared_memory_object(open_or_create, topic.c_str(), read_write);
    shm_obj.truncate(sizeof(SharedQueue<queue_size>));
    region_ = std::make_shared<mapped_region>(shm_obj, read_write);
    auto *base_addr = reinterpret_cast<uint8_t *>(region_->get_address());

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
    DEBUG("init_info: start");
    sh_q_->info_mutex.lock();
    if (sh_q_->init != INFO_INIT) {
      sh_q_->init = INFO_INIT;
      sh_q_->counter = 0;
    }
    sh_q_->info_mutex.unlock();
  }

  bool write(void *data, size_t size) {
    DEBUG("Writing: start");
    int pos = sh_q_->counter.fetch_add(1);
    pos &= queue_size - 1;  // modulo for power of 2
    sh_q_->lock(pos);

    if (size > raw_buf_->get_free_memory()) {
      auto new_size = raw_buf_->get_size() * 2 + size;
      raw_buf_->grow((topic_ + "Raw").c_str(), new_size);
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

  bool read(void *src, uint32_t pos, bool overwrite = true) {
    DEBUG("Reading: start");
    pos &= queue_size - 1;
    sh_q_->lock_sharable(pos);

    auto idx = &(sh_q_->__array[pos]);
    if (idx->empty) return false;
    void *dst = raw_buf_->get_address_from_handle(idx->addr_hdl);
    std::memcpy(src, dst, idx->size);

    sh_q_->unlock_sharable(pos);
    return true;
  }

  inline __attribute__((always_inline)) uint32_t counter() {
    // This forces the function to be inlined allowing to be
    // atomic without contention
    return sh_q_->counter.load();
  }

 private:
  std::string topic_;

  std::shared_ptr<mapped_region> region_;
  std::shared_ptr<managed_shared_memory> raw_buf_;

  SharedQueue<queue_size> *sh_q_;
};
}  // namespace shm
#endif  // SHADERMAR_MEMORY_H
