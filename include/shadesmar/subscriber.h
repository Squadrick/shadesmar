#include <utility>

//
// Created by squadrick on 8/3/19.
//

#ifndef shadesmar_SUBSCRIBER_H
#define shadesmar_SUBSCRIBER_H

#include <cstdint>
#include <cstring>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <shadesmar/memory.h>
#include <shadesmar/message.h>

namespace shm {
template <typename msgT, uint32_t queue_size>
class Subscriber {
  static_assert(std::is_base_of<msg::BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");

 public:
  Subscriber(std::string topic_name,
             std::function<void(const std::shared_ptr<msgT> &)> callback,
             bool reference_passing = false)
      : topic_name_(std::move(topic_name)),
        spinning_(false),
        reference_passing_(reference_passing),
        counter_(0) {
    // TODO: Fix contention
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    callback_ = std::move(callback);
    mem_ = std::make_unique<Memory<queue_size>>(topic_name_);
  }

  void spin() {
    if (spinning_) return;
    spinning_ = true;
    //    spin_thread_ = std::make_shared<std::thread>(&Subscriber::__spin,
    //    this);
    spin_thread_ = new std::thread(&Subscriber::__spin, this);
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
      msgpack::object_handle oh;
      std::shared_ptr<msgT> msg = std::make_shared<msgT>();
      if (reference_passing_) {
        void *raw_msg;
        uint32_t size;
        mem_->read(&raw_msg, size, counter_);
        oh = msgpack::unpack(reinterpret_cast<const char *>(raw_msg), size);
        free(raw_msg);
      } else {
        mem_->read(oh, counter_);
      }

      oh.get().convert(*msg);
      callback_(msg);
      counter_++;
    }
  }

  void shutdown() {
    // stop spinning
    if (!spinning_) return;
    spinning_ = false;
    spin_thread_->join();

    delete spin_thread_;
  }

 private:
  void __spin() {
    while (spinning_) {
      spinOnce();
    }
  }

  std::string topic_name_;

  std::unique_ptr<Memory<queue_size>> mem_;
  std::function<void(const std::shared_ptr<msgT> &)> callback_;

  bool spinning_, reference_passing_;

  //  std::shared_ptr<std::thread> spin_thread_;
  std::thread *spin_thread_{};

  uint32_t counter_;
};
}  // namespace shm
#endif  // shadesmar_SUBSCRIBER_H
