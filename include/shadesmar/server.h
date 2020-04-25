//
// Created by squadrick on 17/12/19.
//

#ifndef SHADESMAR_SERVER_H
#define SHADESMAR_SERVER_H

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

#include <shadesmar/channel.h>
#include <shadesmar/template_magic.h>

namespace shm::rpc {

template <class Sig> class Function;

template <class R, class... Args> class Function<R(Args...)> {
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

    if (!channel_.read(oh)) {
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

} // namespace shm::rpc
#endif // SHADESMAR_SERVER_H
