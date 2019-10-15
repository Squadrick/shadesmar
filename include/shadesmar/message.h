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

#define SHM_PACK(...) MSGPACK_DEFINE(seq, timestamp, frame_id, __VA_ARGS__);

namespace shm {

enum TimeType { ROS, ROS2, SYSTEM };

class BaseMsg {
 public:
  uint32_t seq{};
  uint64_t timestamp{};
  std::string frame_id{};

  BaseMsg() = default;

  inline void init_time(TimeType tt = SYSTEM) {
    if (tt == ROS) {
      //      auto ros_time = ros::Time::now();
      //      timestamp = ros_time.sec * 1000000000 + ros_time.nsec;
    } else if (tt == ROS2) {
      timestamp = 0;
    } else if (tt == SYSTEM) {
      timestamp = std::chrono::duration_cast<TIMESCALE>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    }
  }
};
}  // namespace shm

#endif  // shadesmar_MESSAGE_H
