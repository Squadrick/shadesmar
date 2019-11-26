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

template <uint32_t queue_size> class SubscriberBase {
public:
  void spinOnce();
  void spin();

  virtual void _subscribe() = 0;

protected:
  SubscriberBase(std::string topic_name);
  std::string topic_name_;
  std::unique_ptr<Memory<queue_size>> topic;
  uint32_t counter{};
};

template <uint32_t queue_size>
class SubscriberBin : public SubscriberBase<queue_size> {
public:
  SubscriberBin(
      std::string topic_name,
      const std::function<void(std::unique_ptr<uint8_t[]>&, size_t)> &callback)
      : SubscriberBase<queue_size>(topic_name), callback_(callback) {}

private:
  const std::function<void(std::unique_ptr<uint8_t[]>&, size_t)> callback_;
  void _subscribe();
};

template <typename msgT, uint32_t queue_size>
class Subscriber : public SubscriberBase<queue_size> {

  static_assert(std::is_base_of<BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");

public:
  Subscriber(std::string topic_name,
             const std::function<void(const std::shared_ptr<msgT> &)> &callback,
             bool extra_copy = false)
      : SubscriberBase<queue_size>(topic_name), callback_(callback),
        extra_copy_(extra_copy) {}

private:
  std::function<void(const std::shared_ptr<msgT> &)> callback_;
  bool extra_copy_;

  void _subscribe();
};

template <uint32_t queue_size>
SubscriberBase<queue_size>::SubscriberBase(std::string topic_name)
    : topic_name_(topic_name) {
  std::this_thread::sleep_for(std::chrono::microseconds(2000));

  #if __cplusplus >= 201703L
    topic = std::make_unique<Memory<queue_size>>(topic_name_);
  #else
    topic = std::unique_ptr<Memory<queue_size>>(new Memory<queue_size>(std::forward<std::string>(topic_name_)));
  #endif

  counter = topic->counter();
}

template <uint32_t queue_size> void SubscriberBase<queue_size>::spinOnce() {
  if (topic->counter() <= counter) {
    // no new messages
    return;
  }

  // TODO: Find a better policy
  // pub_counter must be strictly greater than counter.
  // If they're equal, there have been no new writes.
  // If pub_counter to too far ahead, we need to catch up.
  if (topic->counter() - counter >= queue_size) {
    // If we have fallen behind by the size of the queue
    // in the case of lapping, we go to last existing
    // element in the queue.
    counter = topic->counter() - queue_size + 1;
  }

  _subscribe();
  counter++;
}

template <uint32_t queue_size> void SubscriberBase<queue_size>::spin() {
  while (true) {
    spinOnce();
  }
}

template <uint32_t queue_size> void SubscriberBin<queue_size>::_subscribe() {
  std::unique_ptr<uint8_t[]> msg;
  size_t size = 0;

  bool write_success = this->topic->read_raw(msg, size, this->counter);
  if (!write_success)
    return;

  callback_(msg, size);
}

template <typename msgT, uint32_t queue_size>
void Subscriber<msgT, queue_size>::_subscribe() {

  msgpack::object_handle oh;
  bool write_success;

  if (extra_copy_) {
    write_success = this->topic->read_with_copy(oh, this->counter);
  } else {
    write_success = this->topic->read_without_copy(oh, this->counter);
  }

  if (!write_success)
    return;

  std::shared_ptr<msgT> msg = std::make_shared<msgT>();
  oh.get().convert(*msg);
  callback_(msg);
}

} // namespace shm
#endif // shadesmar_SUBSCRIBER_H
