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

#include <cassert>
#include <chrono>
#include <iostream>

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/dragons.h"
#include "shadesmar/rpc/client.h"
#include "shadesmar/rpc/server.h"
#include "shadesmar/stats.h"
#endif

const char topic[] = "raw_benchmark_topic_rpc";

shm::stats::Welford per_second_lag;

struct Message {
  uint64_t count;
  uint64_t timestamp;
  uint8_t *data;
};

uint64_t current_count = 0;
uint8_t *resp_buffer = nullptr;
uint64_t resp_size = 0;

bool callback(const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
  auto *msg = reinterpret_cast<Message *>(req.ptr);
  auto lag = shm::current_time() - msg->timestamp;
  std::cout << current_count << ", " << msg->count << "\n";
  assert(current_count < msg->count);
  current_count = msg->count;
  per_second_lag.add(lag);
  resp->ptr = resp_buffer;
  resp->size = resp_size;
  return true;
}

void cleanup(shm::memory::Memblock *resp) {}

void server_loop(int seconds) {
  shm::rpc::Server server(topic, callback, cleanup);

  for (int sec = 0; sec < seconds; ++sec) {
    auto start = std::chrono::steady_clock::now();
    for (auto now = start; now < start + std::chrono::seconds(1);
         now = std::chrono::steady_clock::now()) {
      server.serve_once();
    }
    std::cout << per_second_lag << std::endl;
    per_second_lag.clear();
  }
}

void client_loop(int seconds, int vector_size) {
  std::cout << "Number of bytes = " << vector_size << std::endl;
  std::cout << "Time unit = " << TIMESCALE_NAME << std::endl;

  resp_buffer = reinterpret_cast<uint8_t *>(malloc(vector_size));
  std::memset(resp_buffer, 255, vector_size);
  resp_size = vector_size;

  auto *rawptr = malloc(vector_size);
  std::memset(rawptr, 255, vector_size);
  Message *msg = reinterpret_cast<Message *>(rawptr);
  msg->count = 0;

  shm::rpc::Client client(topic);

  auto start = std::chrono::steady_clock::now();
  for (auto now = start; now < start + std::chrono::seconds(seconds);
       now = std::chrono::steady_clock::now()) {
    msg->count++;
    msg->timestamp = shm::current_time();
    shm::memory::Memblock req, resp;
    req.ptr = msg;
    req.size = vector_size;
    client.call(req, &resp);
  }
  free(msg);
}

int main() {
  const int SECONDS = 10;
  const int VECTOR_SIZE = 32 * 1024;
  std::thread server_thread(server_loop, SECONDS);
  std::thread client_thread(client_loop, SECONDS, VECTOR_SIZE);

  server_thread.join();
  client_thread.join();
}
