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
      : fn_(fn){};
  auto serveOnce(msgpack::sbuffer &buf) {
    msgpack::object_handle res = msgpack::unpack(buf.data(), buf.size());
    msgpack::object obj = res.get();
    std::tuple<Args...> args;
    obj.convert(args);
    return std::apply(fn_, args);
  }

private:
  std::function<R(Args...)> fn_;
};

} // namespace shm::rpc
#endif // SHADESMAR_SERVER_H
