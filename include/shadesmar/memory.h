//
// Created by squadrick on 8/2/19.
//

#ifndef SHADERMAR_MEMORY_H
#define SHADERMAR_MEMORY_H

#include <string>
#include <stdint.h>
#include <cstring>

#include <iostream>

#include <sys/types.h>
#include <sys/ipc.h>

#include <boost/scope_exit.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>

#define META_STR "-shadermar-alpha"
#define SAFE_REGION 1024

using namespace boost::interprocess;
namespace shm {
struct MemInfo {
  int init;
  int msg_size;
  int buffer_size;
  int counter;
};

class Memory {
 public:
  Memory(std::string topic, uint32_t msg_size) :
      topic_(topic), msg_size_(msg_size), buffer_size_(1) {

    size_t _size = buffer_size_ * msg_size_ + SAFE_REGION;
    size_t size = _size + sizeof(MemInfo);
    std::shared_ptr<shared_memory_object> mem;

    mem = std::make_shared<shared_memory_object>(open_or_create, topic_.c_str(), read_write);
    mem->truncate(size);
    region_ = std::make_shared<mapped_region>(*mem, read_write);

    info_ = reinterpret_cast<MemInfo *>(static_cast<char *>(region_->get_address()) + size);
    boost::interprocess::scoped_lock<interprocess_upgradable_mutex> lock(info_mutex);
    if (info_->init != 666) {
      // not inited
      info_->init = 1337;
      info_->buffer_size = buffer_size_;
      info_->msg_size = msg_size_;
      info_->counter = 0;
    } else {
      if (info_->msg_size != msg_size_)
        throw "message sizes do not match";
    }
  }

  ~Memory() {
  }

  bool write(void *data) {
    boost::interprocess::scoped_lock<interprocess_upgradable_mutex> lock(mem_mutex);
    std::cout << region_->get_address() << std::endl;
    std::memcpy(region_->get_address(), data, msg_size_);
    return true;
  }

  bool writeGPU(void *data) {
    // implement GPU memcpy here
    return true;
  }

  bool read(void *src, bool overwrite = true) {
    boost::interprocess::sharable_lock<interprocess_upgradable_mutex> lock(mem_mutex);
    if (overwrite) {
      std::cout << region_->get_address() << std::endl;
      std::memcpy(src, region_->get_address(), msg_size_);
    } else {
      // TODO: Non-overwritten
    }
    return true;
  }

  void readGPU(void *src) {
    // implement GPU memcpy here
  }

  void inc() {
    boost::interprocess::scoped_lock<interprocess_upgradable_mutex> lock(info_mutex);
    ++(info_->counter);
  }

  int counter() {
    boost::interprocess::sharable_lock<interprocess_upgradable_mutex> lock(mem_mutex);
    return (info_->counter);
  }

 private:
  void remove_old_shared_memory() {
    std::cout << "removed" << std::endl;
    shared_memory_object::remove(topic_.c_str());
  }

  uint32_t msg_size_, buffer_size_;
  std::string topic_;

  mutable interprocess_upgradable_mutex mem_mutex, info_mutex;

  std::shared_ptr<mapped_region> region_;
  MemInfo *info_;

};
}
#endif //SHADERMAR_MEMORY_H
