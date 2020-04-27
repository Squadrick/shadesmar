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

#include <shadesmar/message.h>
#include <shadesmar/pubsub/topic.h>

namespace shm::pubsub {

template <uint32_t queue_size> class SubscriberBase {
public:
  void spin_once();
  void spin();

  virtual void _subscribe() = 0;

protected:
  SubscriberBase(std::string topic_name, bool extra_copy);
  std::string topic_name_;
  std::unique_ptr<Topic<queue_size>> topic;
  std::atomic<uint32_t> counter{0};
};

template <uint32_t queue_size>
class SubscriberBin : public SubscriberBase<queue_size> {
public:
  SubscriberBin(
      const std::string &topic_name,
      std::function<void(std::unique_ptr<uint8_t[]> &, size_t)> callback)
      : SubscriberBase<queue_size>(topic_name, false),
        callback_(std::move(callback)) {}

private:
  std::function<void(std::unique_ptr<uint8_t[]> &, size_t)> callback_;
  void _subscribe();
};

template <typename msgT, uint32_t queue_size>
class Subscriber : public SubscriberBase<queue_size> {

  static_assert(std::is_base_of<BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");

public:
  Subscriber(const std::string &topic_name,
             std::function<void(const std::shared_ptr<msgT> &)> callback,
             bool extra_copy = false)
      : SubscriberBase<queue_size>(topic_name, extra_copy),
        callback_(std::move(callback)) {}

private:
  std::function<void(const std::shared_ptr<msgT> &)> callback_;
  void _subscribe();
};

template <uint32_t queue_size>
SubscriberBase<queue_size>::SubscriberBase(std::string topic_name,
                                           bool extra_copy)
    : topic_name_(std::move(topic_name)) {

#if __cplusplus >= 201703L
  topic = std::make_unique<Topic<queue_size>>(topic_name_, extra_copy);
#else
  topic = std::unique_ptr<Topic<queue_size>>(new Topic<queue_size>(
      std::forward<std::string>(topic_name_, extra_copy)));
#endif

  counter = topic->counter();
}

template <uint32_t queue_size> void SubscriberBase<queue_size>::spin_once() {
  /*
   * topic's `counter` must be strictly greater than counter.
   * If they're equal, there have been no new writes.
   * If subscriber's counter is too far ahead, we need to catch up.
   */
  if (topic->counter() <= counter) {
    // no new messages
    return;
  }

  if (topic->counter() - counter > queue_size) {
    /*
     * If we have fallen behind by the size of the queue
     * in the case of overlap, we go to last existing
     * element in the queue.
     *
     * Why is the check > (not >=)? This is because in topic's
     * `write` we do a `fetch_add`, so the queue pointer is already
     * ahead of where it last wrote.
     */
    counter = topic->counter() - queue_size;
  }

  _subscribe();
  counter++;
}

template <uint32_t queue_size> void SubscriberBase<queue_size>::spin() {
  // TODO: Spin on different thread
  while (true) {
    spin_once();
  }
}

template <uint32_t queue_size> void SubscriberBin<queue_size>::_subscribe() {
  std::unique_ptr<uint8_t[]> msg;
  size_t size = 0;

  if (!this->topic->read(msg, size, this->counter))
    return;

  callback_(msg, size);
}

template <typename msgT, uint32_t queue_size>
void Subscriber<msgT, queue_size>::_subscribe() {
  msgpack::object_handle oh;

  if (!this->topic->read(oh, this->counter))
    return;

  std::shared_ptr<msgT> msg = std::make_shared<msgT>();
  oh.get().convert(*msg);
  msg->seq = this->counter;
  callback_(msg);
}

} // namespace shm::pubsub
#endif // shadesmar_SUBSCRIBER_H
