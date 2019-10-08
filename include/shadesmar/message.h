//
// Created by squadrick on 8/5/19.
//

#ifndef SHADERMAR_MESSAGE_H
#define SHADERMAR_MESSAGE_H

#include <cstdint>
#include <cstring>

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

#endif  // SHADERMAR_MESSAGE_H
