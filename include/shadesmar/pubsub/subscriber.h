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
class Subscriber {
 public:
  Subscriber(const std::string &topic_name,
             std::function<void(memory::Ptr *)> callback,
             memory::Copier *copier = nullptr);

  memory::Ptr get_message();
  void spin_once();
  void spin();

  std::atomic<uint32_t> counter_{0};

 private:
  std::function<void(memory::Ptr *)> callback_;
  std::string topic_name_;
  std::unique_ptr<Topic<queue_size>> topic_;
};

template <uint32_t queue_size>
Subscriber<queue_size>::Subscriber(const std::string &topic_name,
                                   std::function<void(memory::Ptr *)> callback,
                                   memory::Copier *copier)
    : topic_name_(topic_name), callback_(std::move(callback)) {
  topic_ = std::make_unique<Topic<queue_size>>(topic_name_, copier);
}

template <uint32_t queue_size>
memory::Ptr Subscriber<queue_size>::get_message() {
  /*
   * topic's `counter` must be strictly greater than counter.
   * If they're equal, there have been no new writes.
   * If subscriber's counter is too far ahead, we need to catch up.
   */
  if (topic_->counter() <= counter_) {
    // no new messages
    return memory::Ptr();
  }

  if (topic_->counter() - counter_ > queue_size) {
    /*
     * If we have fallen behind by the size of the queue
     * in the case of overlap, we go to last existing
     * element in the queue.
     *
     * Why is the check > (not >=)? This is because in topic's
     * `write` we do a `fetch_add`, so the queue pointer is already
     * ahead of where it last wrote.
     */
    counter_ = topic_->counter() - queue_size;
  }

  memory::Ptr ptr;
  ptr.free = true;

  if (!topic_->read(&ptr, counter_)) return memory::Ptr();

  return ptr;
}

template <uint32_t queue_size>
void Subscriber<queue_size>::spin_once() {
  memory::Ptr ptr = get_message();

  if (ptr.size == 0) {
    return;
  }

  callback_(&ptr);

  if (ptr.free) {
    auto *copier = topic_->copier();
    if (copier != nullptr) {
      copier->dealloc(ptr.ptr);
    } else {
      free(ptr.ptr);
    }
  }

  counter_++;
}

template <uint32_t queue_size>
void Subscriber<queue_size>::spin() {}

}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_SUBSCRIBER_H_
