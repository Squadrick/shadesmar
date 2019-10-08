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

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <shadesmar/ipc_lock.h>
#include <shadesmar/macros.h>
#include <shadesmar/tmp.h>

using namespace boost::interprocess;
namespace shm {
struct MemInfo {
  int init;
  int msg_size;
  int queue_size;
  std::atomic<uint32_t> counter;
  int pub_count;
  int sub_count;
  IPC_Lock mutex;

  void lock() {
    DEBUG("MemInfo lock: trying");
    mutex.lock();
    DEBUG("MemInfo lock: success");
  }

  void unlock() {
    DEBUG("MemInfo unlock: trying");
    mutex.unlock();
    DEBUG("MemInfo unlock: success");
  }

  void lock_sharable() {
    DEBUG("MemInfo share lock: trying");
    mutex.lock_sharable();
    DEBUG("MemInfo share lock: success");
  }

  void unlock_sharable() {
    DEBUG("MemInfo share unlock: trying");
    mutex.unlock_sharable();
    DEBUG("MemInfo share unlock: success");
  }

  void dump() {
    DEBUG("MemInfo dump");
    DEBUG("msg_size: " << msg_size);
    DEBUG("queue_size: " << queue_size);
    DEBUG("counter: " << counter);
    DEBUG("pub_count: " << pub_count);
    DEBUG("sub_count: " << sub_count);
    mutex.dump();
  }
};

template <uint32_t msg_size, uint32_t queue_size>
struct SharedMem {
  char data[msg_size * queue_size]{};
  IPC_Lock mutex[queue_size];

  void lock(uint32_t idx = 0) {
    DEBUG("SharedMem lock [" << idx << "]: trying");
    mutex[idx].lock();
    DEBUG("SharedMem lock [" << idx << "]: success");
  }

  void unlock(uint32_t idx = 0) {
    DEBUG("SharedMem unlock [" << idx << "]: trying");
    mutex[idx].unlock();
    DEBUG("SharedMem unlock [" << idx << "]: success");
  }

  void lock_sharable(uint32_t idx = 0) {
    DEBUG("SharedMem share lock [" << idx << "]: trying");
    mutex[idx].lock_sharable();
    DEBUG("SharedMem share lock [" << idx << "]: success");
  }

  void unlock_sharable(uint32_t idx = 0) {
    DEBUG("SharedMem share unlock [" << idx << "]: trying");
    mutex[idx].unlock_sharable();
    DEBUG("SharedMem share unlock [" << idx << "]: success");
  }

  void dump() {
    DEBUG("SharedMem dump");
    for (int i = 0; i < queue_size; ++i) {
      DEBUG("Lock " << i);
      mutex[i].dump();
    }
  }
};

template <uint32_t msg_size = 0, uint32_t queue_size = 1>
class Memory {
  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

 public:
  Memory(const std::string &topic, bool publisher)
      : topic_(topic), publisher_(publisher), size_(msg_size * queue_size) {
    // TODO: Has contention
    bool mem_exists = tmp::exists(topic);
    if (!mem_exists) tmp::write_topic(topic);

    size_t __shm_size =
        sizeof(SharedMem<msg_size, queue_size>) + sizeof(MemInfo);

    auto shm_obj =
        shared_memory_object(open_or_create, topic.c_str(), read_write);
    shm_obj.truncate(__shm_size);
    shm_region_ = std::make_shared<mapped_region>(shm_obj, read_write);
    auto *shm_addr = reinterpret_cast<uint8_t *>(shm_region_->get_address());

    if (!mem_exists) {
      mem_ = new (shm_addr) SharedMem<msg_size, queue_size>();
      info_ = new (shm_addr + sizeof(SharedMem<msg_size, queue_size>)) MemInfo;
      init_info();
    } else {
      mem_ = reinterpret_cast<SharedMem<msg_size, queue_size> *>(shm_addr);
      info_ = reinterpret_cast<MemInfo *>(
          shm_addr + sizeof(SharedMem<msg_size, queue_size>));
    }
  }

  ~Memory() {
    if (msg_size == 0) return;
    int pub_count, sub_count;
    std::cout << "~Memory: start\n";
    info_->lock();

    if (publisher_) {
      info_->pub_count -= 1;
    } else {
      info_->sub_count -= 1;
    }

    pub_count = info_->pub_count;
    sub_count = info_->sub_count;

    info_->unlock();

    if (pub_count + sub_count == 0) {
      remove_old_shared_memory();
    }
  }

  void init_info() {
    DEBUG("init_info: start");
    info_->lock();
    if (info_->init != INFO_INIT) {
      info_->init = INFO_INIT;
      info_->queue_size = queue_size;
      info_->msg_size = msg_size;
      info_->counter = 0;
      info_->pub_count = 0;
      info_->sub_count = 0;
    } else {
      if (info_->msg_size != msg_size) {
        std::cerr << "message sizes do not match" << std::endl;
        exit(-1);
      }
      if (info_->queue_size != queue_size) {
        std::cerr << "queue sizes do not match" << std::endl;
      }
    }

    if (publisher_) {
      info_->pub_count += 1;
    } else {
      info_->sub_count += 1;
    }

    info_->unlock();
  }

  bool write(void *data) {
    DEBUG("Writing: start");
    int pos = info_->counter.fetch_add(1);
    pos &= queue_size - 1;
    mem_->lock(pos);
    std::memcpy(mem_->data + pos * msg_size, data, msg_size);
    mem_->unlock(pos);
    return true;
  }

  bool read(void *src, uint32_t pos, bool overwrite = true) {
    DEBUG("Reading: start");
    pos &= queue_size - 1;
    mem_->lock_sharable(pos);
    std::memcpy(src, mem_->data + pos * msg_size, msg_size);
    mem_->unlock_sharable(pos);
    return true;
  }

  inline __attribute__((always_inline)) uint32_t counter() {
    // This forces the function to be inlined allowing to be
    // atomic without contention
    return info_->counter.load();
  }

  void remove_old_shared_memory() {
    shared_memory_object::remove(topic_.c_str());
  }

  void dump() {
    DEBUG("Memory dump for topic: " << topic_);
    DEBUG("msg_size: " << msg_size);
    DEBUG("queue_size: " << queue_size);
    DEBUG("buffer size: " << size_);
    info_->dump();
    mem_->dump();
    DEBUG("\n");
  }

 private:
  uint32_t size_;
  std::string topic_;

  std::shared_ptr<mapped_region> shm_region_;

  SharedMem<msg_size, queue_size> *mem_;
  MemInfo *info_;

  bool publisher_;
};
}  // namespace shm
#endif  // SHADERMAR_MEMORY_H
