//
// Created by squadrick on 7/30/19.
//

#ifndef shadesmar_PUBLISHER_H
#define shadesmar_PUBLISHER_H

#include <cstdint>

#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include <msgpack.hpp>

#include <shadesmar/message.h>
#include <shadesmar/topic.h>

namespace shm {

template <uint32_t queue_size> class PublisherBin {
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

template <typename msgT, uint32_t queue_size> class Publisher {
public:
  explicit Publisher(std::string topic_name);
  bool publish(std::shared_ptr<msgT> msg);
  bool publish(msgT &msg);
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
bool Publisher<msgT, queue_size>::publish(msgT &msg) {
  return publish(&msg);
}

template <typename msgT, uint32_t queue_size>
bool Publisher<msgT, queue_size>::publish(msgT *msg) {
  msg->seq = topic_.counter();
  msgpack::sbuffer buf;
  try {
    msgpack::pack(buf, *msg);
  } catch (...) {
    return false;
  }
  return topic_.write(buf.data(), buf.size());
}

} // namespace shm
#endif // shadesmar_PUBLISHER_H
