/* MIT License

Copyright (c) 2020 Dheeraj R Reddy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

#ifndef INCLUDE_SHADESMAR_PUBSUB_SUBSCRIBER_H_
#define INCLUDE_SHADESMAR_PUBSUB_SUBSCRIBER_H_

#include <cstdint>
#include <cstring>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "shadesmar/memory/copier.h"
#include "shadesmar/message.h"
#include "shadesmar/pubsub/topic.h"

namespace shm::pubsub {

template <uint32_t queue_size>
class SubscriberBase {
 public:
  void spin_once();
  void spin();

  virtual void _subscribe() = 0;

 protected:
  SubscriberBase(std::string topic_name, memory::Copier *copier,
                 bool extra_copy);

  std::string topic_name_;
  std::unique_ptr<Topic<queue_size>> topic;
  std::atomic<uint32_t> counter{0};
};

template <uint32_t queue_size>
class SubscriberBin : public SubscriberBase<queue_size> {
 public:
  SubscriberBin(const std::string &topic_name, memory::Copier *copier,
                std::function<void(memory::Ptr *)> callback)
      : SubscriberBase<queue_size>(topic_name, copier, false),
        callback_(std::move(callback)) {}

 private:
  std::function<void(memory::Ptr *)> callback_;
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
      : SubscriberBase<queue_size>(topic_name, nullptr, extra_copy),
        callback_(std::move(callback)) {}

 private:
  std::function<void(const std::shared_ptr<msgT> &)> callback_;
  void _subscribe();
};

template <uint32_t queue_size>
SubscriberBase<queue_size>::SubscriberBase(std::string topic_name,
                                           memory::Copier *copier,
                                           bool extra_copy)
    : topic_name_(std::move(topic_name)) {
  topic = std::make_unique<Topic<queue_size>>(topic_name_, copier, extra_copy);
  counter = topic->counter();
}

template <uint32_t queue_size>
void SubscriberBase<queue_size>::spin_once() {
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

template <uint32_t queue_size>
void SubscriberBase<queue_size>::spin() {
  // TODO(squadrick): Spin on different thread
  while (true) {
    spin_once();
  }
}

template <uint32_t queue_size>
void SubscriberBin<queue_size>::_subscribe() {
  memory::Ptr ptr;
  ptr.free = true;

  if (!this->topic->read(&ptr, this->counter)) return;

  callback_(&ptr);

  if (ptr.free) {
    this->topic->copier()->dealloc(ptr.ptr);
  }
}

template <typename msgT, uint32_t queue_size>
void Subscriber<msgT, queue_size>::_subscribe() {
  msgpack::object_handle oh;

  if (!this->topic->read(&oh, this->counter)) return;

  std::shared_ptr<msgT> msg = std::make_shared<msgT>();
  oh.get().convert(*msg);
  msg->seq = this->counter;
  callback_(msg);
}

}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_SUBSCRIBER_H_
