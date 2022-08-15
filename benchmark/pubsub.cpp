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

#include <cassert>
#include <chrono>
#include <iostream>

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/dragons.h"
#include "shadesmar/pubsub/publisher.h"
#include "shadesmar/pubsub/subscriber.h"
#include "shadesmar/stats.h"
#endif

const char topic[] = "raw_benchmark_topic_pubsub";

shm::stats::Welford per_second_lag;

struct Message {
  uint64_t count;
  uint64_t timestamp;
  uint8_t *data;
};

uint64_t current_count = 0;

void callback(shm::memory::Memblock *memblock) {
  if (memblock->is_empty()) {
    return;
  }
  auto *msg = reinterpret_cast<Message *>(memblock->ptr);
  auto lag = shm::current_time() - msg->timestamp;
  // assert(current_count == msg->count - 1);
  assert(current_count < msg->count);
  current_count = msg->count;
  per_second_lag.add(lag);
}

void subscribe_loop(int seconds) {
  shm::pubsub::Subscriber sub(topic, callback);

  for (int sec = 0; sec < seconds; ++sec) {
    auto start = std::chrono::steady_clock::now();
    for (auto now = start; now < start + std::chrono::seconds(1);
         now = std::chrono::steady_clock::now()) {
      sub.spin_once();
    }
    std::cout << per_second_lag << std::endl;
    per_second_lag.clear();
  }
}

void publish_loop(int seconds, int vector_size) {
  std::cout << "Number of bytes = " << vector_size << std::endl
            << "Time unit = " << TIMESCALE_NAME << std::endl;

  auto *rawptr = malloc(vector_size);
  std::memset(rawptr, 255, vector_size);
  Message *msg = reinterpret_cast<Message *>(rawptr);
  msg->count = 0;

  shm::pubsub::Publisher pub(topic);

  auto start = std::chrono::steady_clock::now();
  for (auto now = start; now < start + std::chrono::seconds(seconds);
       now = std::chrono::steady_clock::now()) {
    msg->count++;
    msg->timestamp = shm::current_time();
    pub.publish(msg, vector_size);
  }
  free(msg);
}

int main() {
  const int SECONDS = 10;
  const int VECTOR_SIZE = 32 * 1024;
  std::thread subscribe_thread(subscribe_loop, SECONDS);
  std::thread publish_thread(publish_loop, SECONDS, VECTOR_SIZE);

  subscribe_thread.join();
  publish_thread.join();
}
