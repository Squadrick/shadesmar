/* MIT License

Copyright (c) 2020 Dheeraj R Reddy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

#ifndef INCLUDE_SHADESMAR_RPC_CLIENT_H_
#define INCLUDE_SHADESMAR_RPC_CLIENT_H_

#include <iostream>
#include <msgpack.hpp>
#include <string>
#include <tuple>

#include "shadesmar/rpc/channel.h"
#include "shadesmar/template_magic.h"

namespace shm::rpc {
class FunctionCaller {
 public:
  explicit FunctionCaller(const std::string &name);

  template <typename... Args>
  msgpack::object operator()(Args... args);

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

  while (!channel_.write(buf.data(), buf.size())) {
  }

  while (!channel_.read(&reply_oh)) {
  }

  auto reply_obj = reply_oh.get();
  return reply_obj;
}

}  // namespace shm::rpc
#endif  // INCLUDE_SHADESMAR_RPC_CLIENT_H_
