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

#ifndef INCLUDE_SHADESMAR_PUBSUB_SERIALIZED_SUBSCRIBER_H_
#define INCLUDE_SHADESMAR_PUBSUB_SERIALIZED_SUBSCRIBER_H_

#include <memory>
#include <msgpack.hpp>
#include <string>
#include <utility>

#include "shadesmar/pubsub/subscriber.h"

namespace shm::pubsub {

template <typename msgT>
class SerializedSubscriber {
  static_assert(std::is_base_of<BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");

 public:
  SerializedSubscriber(
      const std::string &topic_name,
      std::function<void(const std::shared_ptr<msgT> &)> callback,
      memory::Copier *copier = nullptr);

  void spin();
  void spin_once();

 private:
  std::function<void(const std::shared_ptr<msgT> &)> callback_;
  Subscriber bin_sub_;
};

template <typename msgT>
SerializedSubscriber<msgT>::SerializedSubscriber(
    const std::string &topic_name,
    std::function<void(const std::shared_ptr<msgT> &)> callback,
    memory::Copier *copier)
    : bin_sub_(
          topic_name, [](memory::Memblock *) {}, copier),
      callback_(std::move(callback)) {}

template <typename msgT>
void SerializedSubscriber<msgT>::spin_once() {
  memory::Memblock memblock = bin_sub_.get_message();

  if (memblock.size == 0) {
    return;
  }
  msgpack::object_handle oh = msgpack::unpack(
      reinterpret_cast<const char *>(memblock.ptr), memblock.size);

  std::shared_ptr<msgT> msg = std::make_shared<msgT>();
  oh.get().convert(*msg);
  msg->seq = bin_sub_.counter_;
  callback_(msg);
}

template <typename msgT>
void SerializedSubscriber<msgT>::spin() {}

}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_SERIALIZED_SUBSCRIBER_H_
