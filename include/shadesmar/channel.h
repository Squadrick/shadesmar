//
// Created by squadrick on 20/11/19.
//

#ifndef SHADESMAR_CHANNEL_H
#define SHADESMAR_CHANNEL_H

#include <cstdint>
#include <cstring>

#include <atomic>
#include <iostream>
#include <string>

#include <msgpack.hpp>

#include <shadesmar/ipc_lock.h>
#include <shadesmar/macros.h>
#include <shadesmar/memory.h>
#include <shadesmar/tmp.h>

namespace shm {
class Channel {
public:
  Channel(const std::string &name);

  bool write(uint8_t *ptr, size_t size);

private:
  std::name_;

  uint8_t buffer_;
};
} // namespace shm

#endif // SHADESMAR_CHANNEL_H
