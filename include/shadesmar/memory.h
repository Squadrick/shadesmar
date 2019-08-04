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
  int msg_size;
  int buffer_size;
  int counter;


};

class Memory {
 public:
  Memory(std::string topic, uint32_t msg_size, bool create) :
      topic_(topic), msg_size_(msg_size), buffer_size_(1), create_(create) {

    if (create_) {
      // TODO: Support multiple publishers
      remove_old_shared_memory();
    }
    size_t size = buffer_size_ + msg_size_ + SAFE_REGION;
    std::shared_ptr<shared_memory_object> mem;

    if (create_)
      mem = std::make_shared<shared_memory_object>(create_only, topic_.c_str(), read_write);
    else
      mem = std::make_shared<shared_memory_object>(open_only, topic_.c_str(), read_write);

    mem->truncate(size);

    region_ = std::make_shared<mapped_region>(*mem, read_write);
  }

  ~Memory() {
  }

  bool write(void *data) {
    boost::interprocess::scoped_lock<upgradable_mutex_type> lock(mutex);
    std::memcpy(region_->get_address(), data, msg_size_);
    return true;
  }

  bool writeGPU(void *data) {
    // implement GPU memcpy here
    return true;
  }

  bool read(void *src, bool overwrite = true) {
    boost::interprocess::sharable_lock<upgradable_mutex_type> lock(mutex);
    if (overwrite) {
      std::memcpy(src, region_->get_address(), msg_size_);
    } else {
      // TODO: Non-overwritten
    }
    return true;
  }

  void readGPU(void *src) {
    // implement GPU memcpy here
  }

 private:
  void remove_old_shared_memory() {
    std::cout << "removed" << std::endl;
    shared_memory_object::remove(topic_.c_str());
  }

  uint32_t msg_size_, buffer_size_;
  std::string topic_;
  bool create_;

  typedef interprocess_upgradable_mutex upgradable_mutex_type;
  mutable upgradable_mutex_type mutex;

  std::shared_ptr<mapped_region> region_;


};
}
#endif //SHADERMAR_MEMORY_H
