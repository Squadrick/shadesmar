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

#ifndef INCLUDE_SHADESMAR_PUBSUB_PUBLISHER_H_
#define INCLUDE_SHADESMAR_PUBSUB_PUBLISHER_H_

#include <cstdint>

#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include <msgpack.hpp>

#include "shadesmar/message.h"
#include "shadesmar/pubsub/topic.h"

namespace shm::pubsub {

template <uint32_t queue_size>
class PublisherBin {
 public:
  explicit PublisherBin(std::string topic_name);
  bool publish(void *data, size_t size);

 private:
  std::string topic_name_;
  Topic<queue_size> topic_;
};

template <uint32_t queue_size>
PublisherBin<queue_size>::PublisherBin(std::string topic_name)
    : topic_name_(topic_name), topic_(Topic<queue_size>(topic_name)) {}

template <uint32_t queue_size>
bool PublisherBin<queue_size>::publish(void *data, size_t size) {
  return topic_.write(data, size);
}

template <typename msgT, uint32_t queue_size>
class Publisher {
 public:
  explicit Publisher(std::string topic_name);
  bool publish(std::shared_ptr<msgT> msg);
  bool publish(msgT &msg);  // NOLINT
  bool publish(msgT *msg);

 private:
  std::string topic_name_;
  Topic<queue_size> topic_;
};

template <typename msgT, uint32_t queue_size>
Publisher<msgT, queue_size>::Publisher(std::string topic_name)
    : topic_name_(topic_name), topic_(Topic<queue_size>(topic_name)) {
  static_assert(std::is_base_of<BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");
}

template <typename msgT, uint32_t queue_size>
bool Publisher<msgT, queue_size>::publish(std::shared_ptr<msgT> msg) {
  return publish(msg.get());
}

template <typename msgT, uint32_t queue_size>
bool Publisher<msgT, queue_size>::publish(msgT &msg) {  // NOLINT
  return publish(&msg);
}

template <typename msgT, uint32_t queue_size>
bool Publisher<msgT, queue_size>::publish(msgT *msg) {
  msgpack::sbuffer buf;
  try {
    msgpack::pack(buf, *msg);
  } catch (...) {
    return false;
  }
  return topic_.write(buf.data(), buf.size());
}

}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_PUBLISHER_H_
