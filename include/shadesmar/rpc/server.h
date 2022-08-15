/* MIT License

Copyright (c) 2021 Dheeraj R Reddy

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

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "shadesmar/memory/memory.h"
#include "shadesmar/rpc/channel.h"

namespace shm::rpc {

using Callback =
    std::function<bool(const memory::Memblock& req, memory::Memblock* resp)>;

using Cleanup = std::function<void(memory::Memblock* resp)>;

inline Cleanup empty_cleanup() {
  return [](memory::Memblock* resp) {
    resp->ptr = nullptr;
    resp->size = 0;
  };
}

class Server {
 public:
  Server(const std::string& channel_name, Callback cb);
  Server(const std::string& channel_name, Callback cb, Cleanup cln);
  Server(const std::string& channel_name, Callback cb, Cleanup cln,
         std::shared_ptr<memory::Copier> copier);
  Server(const Server& other) = delete;
  Server(Server&& other);

  bool serve_once();
  void serve();
  void stop();

 private:
  bool process(uint32_t pos) const;
  void cleanup_req(memory::Memblock*) const;
  std::atomic_uint32_t pos_{0};
  std::atomic_bool running_{false};
  Callback callback_;
  Cleanup cleanup_;
  std::string channel_name_;
  std::unique_ptr<Channel> channel_;
};

Server::Server(const std::string& channel_name, Callback cb)
    : channel_name_(channel_name),
      callback_(std::move(cb)),
      cleanup_(std::move(empty_cleanup())) {
  channel_ = std::make_unique<Channel>(channel_name);
}

Server::Server(const std::string& channel_name, Callback cb, Cleanup cleanup)
    : channel_name_(channel_name),
      callback_(std::move(cb)),
      cleanup_(cleanup) {
  channel_ = std::make_unique<Channel>(channel_name);
}

Server::Server(const std::string& channel_name, Callback cb, Cleanup cleanup,
               std::shared_ptr<memory::Copier> copier)
    : channel_name_(channel_name),
      callback_(std::move(cb)),
      cleanup_(cleanup) {
  channel_ = std::make_unique<Channel>(channel_name, copier);
}

Server::Server(Server&& other) {
  callback_ = std::move(other.callback_);
  cleanup_ = std::move(other.cleanup_);
  channel_ = std::move(other.channel_);
}

void Server::cleanup_req(memory::Memblock* req) const {
  // TODO(squadrick): Move this into Channel.
  if (req->is_empty()) return;
  channel_->copier()->dealloc(req->ptr);
  *req = memory::Memblock(nullptr, 0);
}

bool Server::process(uint32_t pos) const {
  memory::Memblock req, resp;

  bool success = channel_->read_server(pos, &req);
  if (!success) {
    cleanup_req(&req);
    return success;
  }
  bool cbSuccess = callback_(req, &resp);
  if (!cbSuccess) {
    cleanup_(&resp);
    resp = memory::Memblock(nullptr, 0);
    cleanup_req(&req);
  }
  success = channel_->write_server(resp, pos);
  cleanup_req(&req);
  cleanup_(&resp);
  return success;
}

bool Server::serve_once() {
  bool success = process(pos_.fetch_add(1));
  return success;
}

void Server::serve() {
  running_ = true;
  while (running_.load()) {
    serve_once();
  }
}

void Server::stop() { running_ = false; }

}  // namespace shm::rpc

#endif  // INCLUDE_SHADESMAR_RPC_SERVER_H_
