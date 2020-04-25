//
// Created by squadrick on 8/5/19.
//

#ifndef shadesmar_MESSAGE_H
#define shadesmar_MESSAGE_H

#include <chrono>
#include <cstdint>
#include <cstring>
#include <utility>

#include <msgpack.hpp>

#include <shadesmar/macros.h>

#define SHM_PACK(...) MSGPACK_DEFINE(seq, timestamp, __VA_ARGS__);

namespace shm {

class BaseMsg {
public:
  uint32_t seq{};
  uint64_t timestamp{};

  BaseMsg() = default;

  void init_time();
};

void BaseMsg::init_time() {
  timestamp = std::chrono::duration_cast<TIMESCALE>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();
}
} // namespace shm

#endif // shadesmar_MESSAGE_H
