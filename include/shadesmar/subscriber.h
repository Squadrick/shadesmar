#include <utility>

//
// Created by squadrick on 8/3/19.
//

#ifndef SHADERMAR_SUBSCRIBER_H
#define SHADERMAR_SUBSCRIBER_H

#include <stdint.h>

#include <cstring>
#include <functional>
#include <memory>
#include <string>

#include <iostream>

#include <shadesmar/memory.h>
#include <thread>

namespace shm {
template <typename T>
class Subscriber {
 public:
  Subscriber(std::string topic_name,
             std::function<void(std::shared_ptr<T>)> callback,
             bool reference_passing = false)
      : topic_name_(std::move(topic_name)),
        spinning_(false),
        msg_(reinterpret_cast<T *>(malloc(sizeof(T)))),
        reference_passing_(reference_passing),
        counter_(0) {
    callback_ = std::move(callback);
    mem_ = std::make_unique<Memory>(topic_name_, sizeof(T), false);
  }

  void spin() {
    spinning_ = true;

    spin_thread_ = std::make_shared<std::thread>([&]() {
      while(spinning_) {
        spinOnce();
      }
    });
  }

  void spinOnce() {
    // no threading
    int pub_count = mem_->counter();
    if (pub_count == counter_) return;
    mem_->read(msg_.get(), true);
    callback_(msg_);
    counter_ = pub_count;
  }

  void shutdown() {
    // stop spinning
    if (!spinning_) return;
    spinning_ = false;
    spin_thread_->join();
  }

 private:
  std::string topic_name_;

  std::unique_ptr<Memory> mem_;
  std::function<void(std::shared_ptr<T>)> callback_;

  bool spinning_, reference_passing_;

  std::shared_ptr<T> msg_;

  std::shared_ptr<std::thread> spin_thread_;

  int counter_;
};
}  // namespace shm
#endif  // SHADERMAR_SUBSCRIBER_H
