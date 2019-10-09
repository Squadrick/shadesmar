//
// Created by squadrick on 8/5/19.
//

#ifndef shadesmar_MESSAGE_H
#define shadesmar_MESSAGE_H

#include <cstdint>
#include <cstring>

#define MSG_SIZE (1024)
namespace shm {

template <uint32_t msg_size>
struct Msg {
  int count = 0;
  uint8_t bytes[msg_size]{};

  void setVal(int val) {
    count = val;
    std::memset(bytes, val, msg_size);
  }
};
}  // namespace shm

#endif  // shadesmar_MESSAGE_H
