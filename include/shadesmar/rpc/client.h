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

#ifndef INCLUDE_SHADESMAR_RPC_CLIENT_H_
#define INCLUDE_SHADESMAR_RPC_CLIENT_H_

#include <memory>
#include <string>
#include <utility>

#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/memory.h"
#include "shadesmar/rpc/channel.h"

namespace shm::rpc {
class Client {
 public:
  explicit Client(const std::string &channel_name);
  Client(const std::string &channel_name,
         std::shared_ptr<memory::Copier> copier);
  Client(const Client &) = delete;
  Client(Client &&);

  bool call(const memory::Memblock &req, memory::Memblock *resp) const;
  bool send(const memory::Memblock &req, uint32_t *pos) const;
  bool recv(uint32_t pos, memory::Memblock *resp) const;
  void free_resp(memory::Memblock *resp) const;

 private:
  std::string channel_name_;
  std::unique_ptr<Channel> channel_;
};

Client::Client(const std::string &channel_name) : channel_name_(channel_name) {
  channel_ = std::make_unique<Channel>(channel_name);
}

Client::Client(const std::string &channel_name,
               std::shared_ptr<memory::Copier> copier)
    : channel_name_(channel_name) {
  channel_ = std::make_unique<Channel>(channel_name, copier);
}

Client::Client(Client &&other) {
  channel_name_ = other.channel_name_;
  channel_ = std::move(other.channel_);
}

bool Client::call(const memory::Memblock &req, memory::Memblock *resp) const {
  uint32_t pos;

  bool success = send(req, &pos);
  if (!success) return success;
  return recv(pos, resp);
}

bool Client::send(const memory::Memblock &req, uint32_t *pos) const {
  return channel_->write_client(req, pos);
}

bool Client::recv(uint32_t pos, memory::Memblock *resp) const {
  return channel_->read_client(pos, resp);
}

void Client::free_resp(memory::Memblock *resp) const {
  channel_->copier()->dealloc(resp->ptr);
  resp->ptr = nullptr;
  resp->size = 0;
}

}  // namespace shm::rpc

#endif  // INCLUDE_SHADESMAR_RPC_CLIENT_H_
