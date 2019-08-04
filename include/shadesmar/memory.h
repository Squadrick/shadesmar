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
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>
#include <boost/scope_exit.hpp>

#define SAFE_REGION 1024
#define INFO_INIT 1337

using namespace boost::interprocess;
namespace shm {
struct MemInfo {
  int init;
  int msg_size;
  int buffer_size;
  int counter;
  int pub_count;
  int sub_count;
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
    size_ = sizeof(MemInfo) + buffer_size_ * msg_size_ + SAFE_REGION;
    std::shared_ptr<shared_memory_object> mem;

    mem = std::make_shared<shared_memory_object>(open_or_create, topic_.c_str(),
                                                 read_write);
    mem->truncate(size_);
    region_ = std::make_shared<mapped_region>(*mem, read_write);

    init_info();
  }

  ~Memory() {
    if (msg_size_ == 0) return;
    int pub_count, sub_count;
    {
      boost::interprocess::scoped_lock<interprocess_upgradable_mutex> lock(
          info_mutex);

      if (publisher_) {
        info_->pub_count -= 1;
      } else {
        info_->sub_count -= 1;
      }

      pub_count = info_->pub_count;
      sub_count = info_->sub_count;
    }

    if (pub_count + sub_count == 0) {
      remove_old_shared_memory();
    }
  }

  void init_info() {
    info_ = reinterpret_cast<MemInfo *>(region_->get_address());
    boost::interprocess::scoped_lock<interprocess_upgradable_mutex> lock(
        info_mutex);
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
  }

  bool write(void *data) {
    auto addr = static_cast<char *>(region_->get_address()) + sizeof(MemInfo);
    boost::interprocess::scoped_lock<interprocess_upgradable_mutex> lock(
        mem_mutex);
    std::memcpy(addr, data, msg_size_);
    return true;
  }

  bool writeGPU(void *data) {
    // implement GPU memcpy here
    return true;
  }

  bool read(void *src, bool overwrite = true) {
    auto addr = static_cast<char *>(region_->get_address()) + sizeof(MemInfo);
    boost::interprocess::sharable_lock<interprocess_upgradable_mutex> lock(
        mem_mutex);
    if (overwrite) {
      std::memcpy(src, addr, msg_size_);
    } else {
      // TODO: Non-overwritten
    }
    return true;
  }

  void readGPU(void *src) {
    // implement GPU memcpy here
  }

  void inc() {
    boost::interprocess::scoped_lock<interprocess_upgradable_mutex> lock(
        info_mutex);
    ++(info_->counter);
  }

  int counter() {
    boost::interprocess::sharable_lock<interprocess_upgradable_mutex> lock(
        mem_mutex);
    return (info_->counter);
  }

  void remove_old_shared_memory() {
    //    std::memset(region_->get_address(), 0, size_);
    shared_memory_object::remove(topic_.c_str());
  }

 private:
  uint32_t msg_size_, buffer_size_, size_;
  std::string topic_;

  mutable interprocess_upgradable_mutex mem_mutex, info_mutex;

  std::shared_ptr<mapped_region> region_;
  MemInfo *info_;

  bool publisher_;
};
}  // namespace shm
#endif  // SHADERMAR_MEMORY_H
