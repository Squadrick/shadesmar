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

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "shadesmar/memory/copier.h"
#include "shadesmar/pubsub/topic.h"

namespace shm::pubsub {

class Subscriber {
 public:
  Subscriber(const std::string &topic_name,
             std::function<void(memory::Memblock *)> callback);
  Subscriber(const std::string &topic_name,
             std::function<void(memory::Memblock *)> callback,
             std::shared_ptr<memory::Copier> copier);

  Subscriber(const Subscriber &other) = delete;

  Subscriber(Subscriber &&other);

  memory::Memblock get_message();
  void spin_once();
  void spin();
  void stop();

  std::atomic<uint32_t> counter_{0};

 private:
  std::atomic_bool running_{false};
  std::function<void(memory::Memblock *)> callback_;
  std::string topic_name_;
  std::unique_ptr<Topic> topic_;
};

Subscriber::Subscriber(const std::string &topic_name,
                       std::function<void(memory::Memblock *)> callback)
    : topic_name_(topic_name), callback_(std::move(callback)) {
  topic_ = std::make_unique<Topic>(topic_name_);
}

Subscriber::Subscriber(const std::string &topic_name,
                       std::function<void(memory::Memblock *)> callback,
                       std::shared_ptr<memory::Copier> copier)
    : topic_name_(topic_name), callback_(std::move(callback)) {
  topic_ = std::make_unique<Topic>(topic_name_, copier);
}

Subscriber::Subscriber(Subscriber &&other) {
  callback_ = std::move(other.callback_);
  topic_ = std::move(other.topic_);
}

memory::Memblock Subscriber::get_message() {
  /*
   * topic's `counter` must be strictly greater than counter.
   * If they're equal, there have been no new writes.
   * If subscriber's counter is too far ahead, we need to catch up.
   */
  if (topic_->counter() <= counter_) {
    // no new messages
    return memory::Memblock();
  }

  if (topic_->counter() - counter_ >= topic_->queue_size()) {
    /*
     * Why is the check >= (not >)? This is because in topic's
     * `write` we do an `inc` at the end, so the write head
     * (topic_->counter()) is currently under-write.
     *
     * If we have fallen behind by the size of the queue
     * in the case of overlap, we go to last existing
     * element in the queue.
     *
     *
     * Rather than going to the last valid location:
     *    topic_->counter() - topic_->queue_size()
     * We move to a more optimistic location (see `jumpahead`), to prevent
     * hitting a case of always trying to keep up with the publisher.
     */
    counter_ = jumpahead(topic_->counter(), topic_->queue_size());
  }

  memory::Memblock memblock;
  memblock.free = true;

  if (!topic_->read(&memblock, &counter_)) {
    return memory::Memblock();
  }

  return memblock;
}

// Not thread-safe. Should be called from a single thread.
void Subscriber::spin_once() {
  memory::Memblock memblock = get_message();

  if (memblock.is_empty()) {
    return;
  }

  callback_(&memblock);
  counter_++;

  if (memblock.free) {
    topic_->copier()->dealloc(memblock.ptr);
    memblock.ptr = nullptr;
    memblock.size = 0;
  }
}

void Subscriber::spin() {
  running_ = true;
  while (running_.load()) {
    spin_once();
  }
}

void Subscriber::stop() { running_ = false; }

}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_SUBSCRIBER_H_
