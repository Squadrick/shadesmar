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

#define SAFE_REGION 1024  // 1kB
#define INFO_INIT 1337
#define MAX_BUFFER 1 << 27  // 128 MB

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
  char data[MAX_BUFFER];
  interprocess_upgradable_mutex mutex;
};

class Memory {
 public:
  Memory(std::string topic, uint32_t msg_size, bool publisher)
      : topic_(topic),
        msg_size_(msg_size),
        buffer_size_(1),
        publisher_(publisher) {
    if (msg_size == 0) {
      remove_old_shared_memory();
      return;
    }

    std::string mem_name = topic_ + "Mem";
    std::string info_name = topic_ + "Info";

    auto __mem =
        shared_memory_object(open_or_create, mem_name.c_str(), read_write);
    __mem.truncate(sizeof(SharedMem));
    mem_region_ = std::make_shared<mapped_region>(__mem, read_write);
    mem_ = new (mem_region_->get_address()) SharedMem;

    auto __info =
        shared_memory_object(open_or_create, info_name.c_str(), read_write);
    __info.truncate(sizeof(MemInfo));
    info_region_ = std::make_shared<mapped_region>(__info, read_write);
    info_ = new (info_region_->get_address()) MemInfo;

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
      if (info_->msg_size != msg_size_) throw "message sizes do not match";
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

  std::shared_ptr<mapped_region> mem_region_, info_region_;
  SharedMem *mem_;
  MemInfo *info_;

  bool publisher_;
};
}  // namespace shm
#endif  // SHADERMAR_MEMORY_H
