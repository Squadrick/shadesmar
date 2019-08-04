//
// Created by squadrick on 7/30/19.
//

#ifndef SHADERMAR_PUBLISHER_H
#define SHADERMAR_PUBLISHER_H

#include <stdint.h>

#include <cstring>
#include <memory>
#include <string>

#include <iostream>

#include <shadesmar/memory.h>

namespace shm {
template <typename T>
class Publisher {
 public:
  Publisher(std::string topic_name, uint32_t buffer_size)
      : topic_name_(topic_name), buffer_size_(buffer_size) {
    mem_ = std::make_shared<Memory>(topic_name, sizeof(T), true);
  }

  bool publish(std::shared_ptr<T> msg) { return publish(msg.get()); }

  bool publish(T &msg) { return publish(&msg); }

  bool publish(T *msg) {
    std::cout << "Publishing " << *msg << std::endl;
    bool success = mem_->write(msg);
    mem_->inc();
    return success;
  }

 private:
  std::string topic_name_;
  uint32_t buffer_size_;

  std::shared_ptr<Memory> mem_;
};
}  // namespace shm
#endif  // SHADERMAR_PUBLISHER_H
