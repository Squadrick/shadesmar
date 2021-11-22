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

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/memory/memory.h"
#include "shadesmar/rpc/client.h"
#include "shadesmar/rpc/server.h"
#endif

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

void free_cleanup(shm::memory::Memblock *resp) {
  free(resp->ptr);
  resp->ptr = nullptr;
  resp->size = 0;
}

TEST_CASE("basic") {
  std::string channel_name = "basic";
  shm::rpc::Client client(channel_name);
  shm::rpc::Server server(
      channel_name,
      [](const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
        resp->ptr = malloc(1024);
        resp->size = 1024;
        return true;
      },
      free_cleanup);
  shm::memory::Memblock req, resp;
  uint32_t pos;
  REQUIRE(client.send(req, &pos));
  REQUIRE(server.serve_once());
  REQUIRE(client.recv(pos, &resp));
  REQUIRE(!resp.is_empty());
  REQUIRE(resp.size == 1024);
  client.free_resp(&resp);
  REQUIRE(resp.is_empty());
}

TEST_CASE("failure") {
  std::string channel_name = "failure";
  shm::rpc::Client client(channel_name);
  shm::rpc::Server server(
      channel_name,
      [](const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
        resp->ptr = malloc(1024);
        resp->size = 1024;
        return false;
      },
      free_cleanup);
  shm::memory::Memblock req, resp;
  uint32_t pos;
  REQUIRE(client.send(req, &pos));
  REQUIRE(server.serve_once());
  REQUIRE(client.recv(pos, &resp));
  REQUIRE(resp.size == 0);
  client.free_resp(&resp);
}

TEST_CASE("single_message") {
  char value = 127;
  size_t size = 10;
  std::string channel_name = "single_message";
  shm::rpc::Client client(channel_name);
  shm::rpc::Server server(
      channel_name,
      [](const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
        resp->ptr = malloc(req.size);
        resp->size = req.size;
        std::memset(resp->ptr, static_cast<char *>(req.ptr)[0], req.size);
        return true;
      },
      free_cleanup);

  uint32_t pos;
  {
    shm::memory::Memblock req;
    req.ptr = malloc(size);
    req.size = size;
    std::memset(req.ptr, value, req.size);
    REQUIRE(client.send(req, &pos));
    free(req.ptr);
  }
  REQUIRE(server.serve_once());
  {
    shm::memory::Memblock resp;
    REQUIRE(client.recv(pos, &resp));
    REQUIRE(resp.size == size);
    REQUIRE(static_cast<char *>(resp.ptr)[0] == value);
    client.free_resp(&resp);
  }
}

TEST_CASE("multiple_messages") {
  std::string channel_name = "multiple_messages";

  std::vector<char> messages = {1, 2, 3, 4, 5};
  std::vector<char> returns;
  std::vector<char> expected = {2, 4, 6, 8, 10};

  shm::rpc::Client client(channel_name);
  shm::rpc::Server server(
      channel_name,
      [](const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
        resp->ptr = malloc(req.size);
        resp->size = req.size;
        std::memset(resp->ptr, 2 * (static_cast<char *>(req.ptr)[0]),
                    req.size);
        return true;
      },
      free_cleanup);

  for (auto message : messages) {
    uint32_t pos;
    shm::memory::Memblock req;
    req.ptr = malloc(sizeof(char));
    req.size = sizeof(char);
    std::memset(req.ptr, message, req.size);
    REQUIRE(client.send(req, &pos));
    REQUIRE(server.serve_once());
    free(req.ptr);
    shm::memory::Memblock resp;
    REQUIRE(client.recv(pos, &resp));
    REQUIRE(resp.size == sizeof(char));
    returns.push_back(static_cast<char *>(resp.ptr)[0]);
    client.free_resp(&resp);
  }
  REQUIRE(returns == expected);
}

TEST_CASE("mutliple_clients") {
  std::string channel_name = "multiple_clients";

  std::vector<char> messages = {1, 2, 3, 4, 5};
  std::vector<char> returns;
  std::vector<char> expected = {2, 4, 6, 8, 10};

  shm::rpc::Client client1(channel_name);
  shm::rpc::Client client2(channel_name);
  shm::rpc::Client *client;
  shm::rpc::Server server(
      channel_name,
      [](const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
        resp->ptr = malloc(req.size);
        resp->size = req.size;
        std::memset(resp->ptr, 2 * (static_cast<char *>(req.ptr)[0]),
                    req.size);
        return true;
      },
      free_cleanup);

  for (auto message : messages) {
    client = message % 2 ? &client1 : &client2;
    uint32_t pos;
    shm::memory::Memblock req;
    req.ptr = malloc(sizeof(char));
    req.size = sizeof(char);
    std::memset(req.ptr, message, req.size);
    REQUIRE(client->send(req, &pos));
    REQUIRE(server.serve_once());
    free(req.ptr);
    shm::memory::Memblock resp;
    REQUIRE(client->recv(pos, &resp));
    REQUIRE(resp.size == sizeof(char));
    returns.push_back(static_cast<char *>(resp.ptr)[0]);
    client->free_resp(&resp);
  }
  REQUIRE(returns == expected);
}

TEST_CASE("serve_on_separate_thread") {
  std::string channel_name = "serve_on_separate_thread";

  std::vector<char> messages = {1, 2, 3, 4, 5};
  std::vector<char> returns;
  std::vector<char> expected = {2, 4, 6, 8, 10};

  shm::rpc::Server server(
      channel_name,
      [](const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
        resp->ptr = malloc(req.size);
        resp->size = req.size;
        std::memset(resp->ptr, 2 * (static_cast<char *>(req.ptr)[0]),
                    req.size);
        return true;
      },
      free_cleanup);
  std::thread th([&]() { server.serve(); });

  shm::rpc::Client client(channel_name);
  for (auto message : messages) {
    shm::memory::Memblock req, resp;
    req.ptr = malloc(sizeof(char));
    req.size = sizeof(char);
    std::memset(req.ptr, message, req.size);
    REQUIRE(client.call(req, &resp));
    returns.push_back(static_cast<char *>(resp.ptr)[0]);
    client.free_resp(&resp);
  }
  server.stop();
  th.join();
  REQUIRE(returns == expected);
}
