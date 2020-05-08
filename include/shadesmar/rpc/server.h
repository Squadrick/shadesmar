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

#ifndef INCLUDE_SHADESMAR_RPC_SERVER_H_
#define INCLUDE_SHADESMAR_RPC_SERVER_H_

#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "shadesmar/rpc/channel.h"
#include "shadesmar/template_magic.h"

namespace shm::rpc {

template <class Sig>
class Function;

template <class R, class... Args>
class Function<R(Args...)> {
 public:
  Function(const std::string &fn_name, std::function<R(Args...)> fn)
      : channel_(Channel(fn_name, false)), fn_(fn) {}

  bool serve_once() {
    if (channel_.counter() <= channel_.idx_) {
      return false;
    }

    msgpack::object_handle oh;
    msgpack::sbuffer buf;
    std::tuple<Args...> args;

    if (!channel_.read(&oh)) {
      return false;
    }

    oh.get().convert(args);

    auto result = std::apply(fn_, args);
    msgpack::pack(buf, result);

    return channel_.write(buf.data(), buf.size());
  }

 private:
  std::function<R(Args...)> fn_;
  Channel channel_;
};

}  // namespace shm::rpc
#endif  // INCLUDE_SHADESMAR_RPC_SERVER_H_
