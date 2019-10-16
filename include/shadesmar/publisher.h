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

#include <shadesmar/memory.h>

namespace shm {
template <typename msgT, uint32_t queue_size> class Publisher {
  static_assert(std::is_base_of<BaseMsg, msgT>::value,
                "msgT must derive from BaseMsg");

public:
  explicit Publisher(std::string topic_name) : topic_name_(topic_name) {
    mem_ = std::make_shared<Memory<queue_size>>(topic_name);
  }

  bool publish(std::shared_ptr<msgT> msg) { return publish(msg.get()); }

  bool publish(msgT &msg) { return publish(&msg); }

  bool publish(msgT *msg) {
    uint32_t seq = mem_->fetch_inc_counter();
    msg->seq = seq;
    bool success;
    TIMEIT(
        {
          msgpack::sbuffer buf;
          msgpack::pack(buf, *msg);
          success = mem_->write(buf.data(), seq, buf.size());
        },
        "PubWrite");
    return success;
  }

private:
  std::string topic_name_;

  std::shared_ptr<Memory<queue_size>> mem_;
};
} // namespace shm
#endif // shadesmar_PUBLISHER_H
