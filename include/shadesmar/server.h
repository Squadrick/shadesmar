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

  bool serveOnce() {
    // check for new message
    int32_t cc = channel_.counter();
    if (cc <= channel_.idx_) {
      return false;
    }

    msgpack::object_handle oh;
    msgpack::sbuffer buf;
    std::tuple<Args...> args;

    DEBUG("S: Input found @ " << channel_.idx_);
    if (!channel_.read(oh, channel_.idx_)) {
      return false;
    }

    try {
      oh.get().convert(args);
    } catch (...) {
      // TODO: We cycle around the queue, and read the older results
      return false;
    }
    DEBUG("S: Function Call");
    auto result = std::apply(fn_, args);

    msgpack::pack(buf, result);

    DEBUG("S: Writing output");
    if (!channel_.write(buf.data(), buf.size())) {
      return false;
    }

    DEBUG("S: RPC success");
    channel_.idx_++;

    return true;
  }

private:
  std::function<R(Args...)> fn_;
  Channel channel_;
};

} // namespace shm::rpc
#endif // SHADESMAR_SERVER_H
