#include <utility>

//
// Created by squadrick on 8/3/19.
//

#ifndef SHADERMAR_SUBSCRIBER_H
#define SHADERMAR_SUBSCRIBER_H

#include <cstdint>
#include <cstring>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <shadesmar/memory.h>

namespace shm {
template <typename T, uint32_t queue_size>
class Subscriber {
 public:
  Subscriber(std::string topic_name,
             std::function<void(const std::shared_ptr<T> &)> callback,
             bool reference_passing = false)
      : topic_name_(std::move(topic_name)),
        spinning_(false),
        msg_(reinterpret_cast<T *>(malloc(sizeof(T)))),
        reference_passing_(reference_passing),
        counter_(0) {
    callback_ = std::move(callback);
    mem_ = std::make_unique<Memory<queue_size>>(topic_name_);
  }

  void spin() {
    spinning_ = true;

    spin_thread_ = std::make_shared<std::thread>([this]() {
      while (spinning_) {
        spinOnce();
      }
    });
  }

  void spinOnce() {
    // poll the buffer once

    int pub_counter = mem_->counter();
    if (pub_counter > counter_) {
      // pub_counter must be strictly greater than counter_
      // if they're equal, there have been no new writes
      // if pub_counter to too far ahead, we need to catch up
      if (pub_counter - counter_ >= queue_size) {
        // if we have fallen behind by the size of the queue
        // in the case of lapping, we go to last existing
        // element in the queue
        counter_ = pub_counter - queue_size + 1;
      }
      if (mem_->read(msg_.get(), counter_, true)) {
        callback_(msg_);
      }
      counter_++;
    }
  }

  void shutdown() {
    // stop spinning
    if (!spinning_) return;
    spinning_ = false;
    spin_thread_->join();
  }

 private:
  std::string topic_name_;

  std::unique_ptr<Memory<queue_size>> mem_;
  std::function<void(const std::shared_ptr<T> &)> callback_;

  bool spinning_, reference_passing_;

  std::shared_ptr<T> msg_;

  std::shared_ptr<std::thread> spin_thread_;

  uint32_t counter_;
};
}  // namespace shm
#endif  // SHADERMAR_SUBSCRIBER_H
