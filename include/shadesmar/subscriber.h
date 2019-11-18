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

template <uint32_t queue_size> class SubscriberBin {
public:
  SubscriberBin(std::string topic_name,
                const std::function<void(void *, size_t)> &callback);
  void spinOnce();

private:
  std::string topic_name_;
  std::unique_ptr<Memory<queue_size>> mem_;
  const std::function<void(void *, size_t)> callback_;
  uint32_t counter_;
};

template <uint32_t queue_size>
SubscriberBin<queue_size>::SubscriberBin(
    std::string topic_name, const std::function<void(void *, size_t)> &callback)
    : topic_name_(std::move(topic_name)), counter_(0), callback_(callback) {
  mem_ = std::make_unique<Memory<queue_size>>(topic_name_);
}

template <uint32_t queue_size> void SubscriberBin<queue_size>::spinOnce() {

  int pub_counter = mem_->counter();
  if (pub_counter > counter_) {
    // TODO: Find a better policy
    // pub_counter must be strictly greater than counter_.
    // If they're equal, there have been no new writes.
    // If pub_counter to too far ahead, we need to catch up.
    if (pub_counter - counter_ >= queue_size) {
      // If we have fallen behind by the size of the queue
      // in the case of lapping, we go to last existing
      // element in the queue.
      counter_ = pub_counter - queue_size + 1;
    }

    void *msg = nullptr;
    size_t size = 0;
    bool write_success = mem_->read_raw(&msg, size, counter_);
    if (!write_success)
      return;

    callback_(msg, size);
    counter_++;
  }
}

template <typename msgT, uint32_t queue_size> class Subscriber {
public:
  Subscriber(std::string topic_name,
             const std::function<void(const std::shared_ptr<msgT> &)> &callback,
             bool extra_copy = false);
  void spin();
  void spinOnce();

private:
  std::string topic_name_;
  std::unique_ptr<Memory<queue_size>> mem_;
  std::function<void(const std::shared_ptr<msgT> &)> callback_;
  bool extra_copy_;
  uint32_t counter_;
};

template <typename msgT, uint32_t queue_size>
Subscriber<msgT, queue_size>::Subscriber(
    std::string topic_name,
    const std::function<void(const std::shared_ptr<msgT> &)> &callback,
    bool extra_copy)
    : topic_name_(std::move(topic_name)), callback_(callback),
      extra_copy_(extra_copy), counter_(0) {

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
    // pub_counter must be strictly greater than counter_.
    // If they're equal, there have been no new writes.
    // If pub_counter to too far ahead, we need to catch up.
    if (pub_counter - counter_ >= queue_size) {
      // If we have fallen behind by the size of the queue
      // in the case of lapping, we go to last existing
      // element in the queue.
      counter_ = pub_counter - queue_size + 1;
    }

    msgpack::object_handle oh;
    bool write_success;

    if (extra_copy_) {
      write_success = mem_->read_with_copy(oh, counter_);
    } else {
      write_success = mem_->read_without_copy(oh, counter_);
    }

    if (!write_success)
      return;

    std::shared_ptr<msgT> msg = std::make_shared<msgT>();
    oh.get().convert(*msg);
    callback_(msg);
    counter_++;
  }
}

} // namespace shm
#endif // shadesmar_SUBSCRIBER_H
