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

class Server {
 public:
  Server(const std::string& channel_name, Callback cb);
  Server(const std::string& channel_name, Callback cb,
         std::shared_ptr<memory::Copier> copier);
  Server(const Server& other) = delete;
  Server(Server&& other);

  void serve_once();
  void serve();
  void stop();

 private:
  bool process(uint32_t pos);
  std::atomic_uint32_t pos_{0};
  std::atomic_bool running_{false};
  Callback callback_;
  std::string channel_name_;
  std::unique_ptr<Channel> channel_;
};

Server::Server(const std::string& channel_name, Callback cb)
    : channel_name_(channel_name), callback_(std::move(cb)) {
  channel_ = std::make_unique<Channel>(channel_name);
}

Server::Server(const std::string& channel_name, Callback cb,
               std::shared_ptr<memory::Copier> copier)
    : channel_name_(channel_name), callback_(std::move(cb)) {
  channel_ = std::make_unique<Channel>(channel_name, copier);
}

Server::Server(Server&& other) {
  callback_ = std::move(other.callback_);
  channel_ = std::move(other.channel_);
}

bool Server::process(uint32_t pos) {
  bool success;
  memory::Memblock req, resp;

  success = channel_->read_server(pos, &req);
  if (!success) return success;
  success = callback_(req, &resp);
  if (!success) return success;
  success = channel_->write_server(resp, pos);
  if (!success) return success;
  return true;
}

void Server::serve_once() {
  bool success = process(pos_.load());
  pos_ += 1;
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
