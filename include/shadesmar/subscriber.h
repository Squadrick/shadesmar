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
#include <utility>

#include <shadesmar/memory.h>
#include <shadesmar/message.h>

namespace shm {
template <typename msgT, uint32_t queue_size> class Subscriber {

public:
  Subscriber(std::string topic_name,
             const std::function<void(const std::shared_ptr<msgT> &)> &callback,
             bool reference_passing = false);

  void spin();
  void spinOnce();

private:
  std::string topic_name_;

  std::unique_ptr<Memory<queue_size>> mem_;
  std::function<void(const std::shared_ptr<msgT> &)> callback_;

  bool reference_passing_;

  uint32_t counter_;
};

template <typename msgT, uint32_t queue_size>
Subscriber<msgT, queue_size>::Subscriber(
    std::string topic_name,
    const std::function<void(const std::shared_ptr<msgT> &)> &callback,
    bool reference_passing)
    : topic_name_(std::move(topic_name)), callback_(callback),
      reference_passing_(reference_passing), counter_(0) {

  static_assert(std::is_base_of<BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");

  // TODO: Fix contention
  std::this_thread::sleep_for(std::chrono::microseconds(2000));
  mem_ = std::make_unique<Memory<queue_size>>(topic_name_);
}

template <typename msgT, uint32_t queue_size>
void Subscriber<msgT, queue_size>::spin() {
  // TODO: spin on a separate thread
  while (true) {
    spinOnce();
  }
}

template <typename msgT, uint32_t queue_size>
void Subscriber<msgT, queue_size>::spinOnce() {
  // poll the buffer once

  int pub_counter = mem_->counter();
  if (pub_counter > counter_) {
    // TODO: Find a better policy
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

    if (reference_passing_) {
      // faster for small (<1MB) messages
      void *raw_msg;
      uint32_t size;
      if (mem_->read(&raw_msg, size, counter_)) {
        try {
          oh = msgpack::unpack(reinterpret_cast<const char *>(raw_msg), size);
          free(raw_msg);
        } catch (...) {
          free(raw_msg);
          return;
        }
      }
    } else {
      // faster for bigger message (>1MB)
      if (!mem_->read(oh, counter_))
        return;
    }

    std::shared_ptr<msgT> msg = std::make_shared<msgT>();
    oh.get().convert(*msg);
    callback_(msg);
    counter_++;
  }
}

} // namespace shm
#endif // shadesmar_SUBSCRIBER_H
