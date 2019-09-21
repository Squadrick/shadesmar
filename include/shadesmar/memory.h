//
// Created by squadrick on 8/2/19.
//

#ifndef SHADERMAR_MEMORY_H
#define SHADERMAR_MEMORY_H

#include <stdint.h>
#include <cstring>
#include <string>

#include <iostream>

#include <sys/ipc.h>
#include <sys/types.h>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>

#include <boost/scope_exit.hpp>

#include <shadesmar/tmp.h>

#define INFO_INIT 1337
#define MAX_BUFFER (1 << 27)  // 128 MB

using namespace boost::interprocess;
namespace shm {
struct MemInfo {
  int init;
  int msg_size;
  int buffer_size;
  int counter;
  int pub_count;
  int sub_count;
  interprocess_upgradable_mutex mutex;
};

struct SharedMem {
  char data[MAX_BUFFER]{};
  interprocess_upgradable_mutex mutex;
};

void *shm_malloc(const std::string &key_name, size_t size) {
  auto __mem = shared_memory_object(open_or_create,
                                    key_name.c_str(), read_write);
  __mem.truncate(size);
  auto __region = new mapped_region(__mem, read_write);
  std::cout << sizeof(mapped_region) << std::endl;
  return __region->get_address();
}

class Memory {
 public:
  Memory(const std::string &topic, uint32_t msg_size, bool publisher)
      : topic_(topic),
        msg_size_(msg_size),
        buffer_size_(1),
        publisher_(publisher) {
    if (msg_size == 0) {
      remove_old_shared_memory();
      return;
    }
    tmp::write_topic(topic);

    size_t __shm_size = sizeof(SharedMem) + sizeof(MemInfo) ;

    auto __shm_obj =
        shared_memory_object(open_or_create, topic.c_str(), read_write);
    __shm_obj.truncate(__shm_size);
    shm_region_ = std::make_shared<mapped_region>(__shm_obj, read_write);
    auto *__shm_addr = reinterpret_cast<uint8_t *>(shm_region_->get_address());

    mem_ = new(__shm_addr) SharedMem;
    info_ = new(__shm_addr + sizeof(SharedMem)) MemInfo;

    init_info();
  }

  ~Memory() {
    if (msg_size_ == 0) return;
    int pub_count, sub_count;
    info_->mutex.lock();

    if (publisher_) {
      info_->pub_count -= 1;
    } else {
      info_->sub_count -= 1;
    }

    pub_count = info_->pub_count;
    sub_count = info_->sub_count;

    info_->mutex.unlock();

    if (pub_count + sub_count == 0) {
      remove_old_shared_memory();
    }
  }

  void init_info() {
    info_->mutex.lock();
    if (info_->init != INFO_INIT) {
      info_->init = INFO_INIT;
      info_->buffer_size = buffer_size_;
      info_->msg_size = msg_size_;
      info_->counter = 0;
      info_->pub_count = 0;
      info_->sub_count = 0;
    } else {
      if (info_->msg_size != msg_size_) {
        std::cerr << "message sizes do not match" << std::endl;
      }
    }

    if (publisher_) {
      info_->pub_count += 1;
    } else {
      info_->sub_count += 1;
    }

    info_->mutex.unlock();
  }

  bool write(void *data) {
    mem_->mutex.lock();
    std::memcpy(mem_->data, data, msg_size_);
    mem_->mutex.unlock();
    return true;
  }

  bool writeGPU(void *data) {
    // implement GPU memcpy here
    return true;
  }

  bool read(void *src, bool overwrite = true) {
    mem_->mutex.lock_sharable();
    std::memcpy(src, mem_->data, msg_size_);
    mem_->mutex.unlock_sharable();
    return true;
  }

  void readGPU(void *src) {
    // implement GPU memcpy here
  }

  void inc() {
    info_->mutex.lock();
    ++(info_->counter);
    info_->mutex.unlock();
  }

  int counter() {
    info_->mutex.lock_sharable();
    int val = info_->counter;
    info_->mutex.unlock_sharable();
    return val;
  }

  void remove_old_shared_memory() {
    //    std::memset(region_->get_address(), 0, size_);
    shared_memory_object::remove(topic_.c_str());
  }

 private:
  uint32_t msg_size_, buffer_size_, size_;
  std::string topic_;

//  std::shared_ptr<mapped_region> mem_region_, info_region_;
  std::shared_ptr<mapped_region> shm_region_;

  SharedMem *mem_;
  MemInfo *info_;

  bool publisher_;
};
}  // namespace shm
#endif  // SHADERMAR_MEMORY_H
