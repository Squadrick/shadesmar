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
