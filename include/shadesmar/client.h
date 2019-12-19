//
// Created by squadrick on 17/12/19.
//

#ifndef SHADESMAR_CLIENT_H
#define SHADESMAR_CLIENT_H

#include <iostream>
#include <string>

#include <msgpack.hpp>

#include <shadesmar/template_magic.h>

namespace shm::rpc {
class Client {
public:
  Client();

  template <typename... Args>
  msgpack::sbuffer call(const std::string &, Args... args);
};

template <typename... Args>
msgpack::sbuffer Client::call(const std::string &, Args... args) {
  msgpack::sbuffer buf;
  std::tuple<Args...> data_tuple(args...);

  msgpack::pack(buf, data_tuple);
  std::cout << buf.data() << std::endl;

  return buf;
}

Client::Client() {}
} // namespace shm::rpc
#endif // SHADESMAR_CLIENT_H
