//
// Created by squadrick on 8/5/19.
//

#ifndef SHADERMAR_MESSAGE_H
#define SHADERMAR_MESSAGE_H

#include <stdint.h>
#define MSGSIZE 1024

namespace shm {

struct Msg {
  int count = 0;
  uint8_t bytes[MSGSIZE];
};
}  // namespace shm

#endif  // SHADERMAR_MESSAGE_H
