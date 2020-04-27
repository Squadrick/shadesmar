//
// Created by squadrick on 17/12/19.
//

#ifndef SHADESMAR_CLIENT_H
#define SHADESMAR_CLIENT_H

#include <iostream>
#include <string>

#include <msgpack.hpp>

#include <shadesmar/rpc/channel.h>
#include <shadesmar/template_magic.h>

namespace shm::rpc {
class FunctionCaller {
public:
  FunctionCaller(const std::string &name);

  template <typename... Args> msgpack::object operator()(Args... args);

private:
  Channel channel_;
};

FunctionCaller::FunctionCaller(const std::string &name)
    : channel_(Channel(name, true)) {}

template <typename... Args>
msgpack::object FunctionCaller::operator()(Args... args) {
  msgpack::sbuffer buf;
  msgpack::object_handle reply_oh;
  std::tuple<Args...> data_tuple(args...);

  msgpack::pack(buf, data_tuple);

  while (!channel_.write(buf.data(), buf.size()))
    ;

  while (!channel_.read(reply_oh))
    ;

  auto reply_obj = reply_oh.get();
  return reply_obj;
}

} // namespace shm::rpc
#endif // SHADESMAR_CLIENT_H
