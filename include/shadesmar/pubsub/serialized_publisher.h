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

#ifndef INCLUDE_SHADESMAR_PUBSUB_SERIALIZED_PUBLISHER_H_
#define INCLUDE_SHADESMAR_PUBSUB_SERIALIZED_PUBLISHER_H_

#include <memory>
#include <string>

#include <msgpack.hpp>

#include "shadesmar/pubsub/publisher.h"

namespace shm::pubsub {

template <typename msgT, uint32_t queue_size>
class SerializedPublisher {
 public:
  explicit SerializedPublisher(const std::string &topic_name,
                               memory::Copier *copier = nullptr);
  bool publish(std::shared_ptr<msgT> msg);
  bool publish(msgT &msg);  // NOLINT
  bool publish(msgT *msg);

 private:
  Publisher<queue_size> bin_pub_;
};

template <typename msgT, uint32_t queue_size>
SerializedPublisher<msgT, queue_size>::SerializedPublisher(
    const std::string &topic_name, memory::Copier *copier)
    : bin_pub_(topic_name, copier) {
  static_assert(std::is_base_of<BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");
}

template <typename msgT, uint32_t queue_size>
bool SerializedPublisher<msgT, queue_size>::publish(std::shared_ptr<msgT> msg) {
  return publish(msg.get());
}

template <typename msgT, uint32_t queue_size>
bool SerializedPublisher<msgT, queue_size>::publish(msgT &msg) {  // NOLINT
  return publish(&msg);
}

template <typename msgT, uint32_t queue_size>
bool SerializedPublisher<msgT, queue_size>::publish(msgT *msg) {
  msgpack::sbuffer buf;
  try {
    msgpack::pack(buf, *msg);
  } catch (...) {
    return false;
  }
  return bin_pub_.publish(buf.data(), buf.size());
}

}  // namespace shm::pubsub

#endif  // INCLUDE_SHADESMAR_PUBSUB_SERIALIZED_PUBLISHER_H_
