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

const char topic[] = "raw_benchmark_topic";
const int SECONDS = 10;
const int VECTOR_SIZE = 32 * 1024;

shm::stats::Welford per_second_lag;

struct Message {
  uint64_t timestamp;
  uint8_t *data;
};

void callback(shm::memory::Memblock *memblock) {
  auto *msg = reinterpret_cast<Message *>(memblock->ptr);
  auto lag = shm::current_time() - msg->timestamp;
  per_second_lag.add(lag);
}

void subscribe_loop() {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  shm::pubsub::Subscriber sub(topic, callback, nullptr);
  int seconds = 0;

  auto start = shm::current_time();
  while (true) {
    sub.spin_once();
    auto end = shm::current_time();
    auto diff = end - start;

    if (diff > TIMESCALE_COUNT) {
      std::cout << "Lag (" << TIMESCALE_NAME << "): " << per_second_lag
                << std::endl;
      if (++seconds == SECONDS) break;
      start = shm::current_time();
      per_second_lag.clear();
    }
  }
}

void publish_loop() {
  shm::pubsub::Publisher pub(topic, nullptr);

  auto *rawptr = malloc(VECTOR_SIZE);
  std::memset(rawptr, 255, VECTOR_SIZE);
  Message *msg = reinterpret_cast<Message *>(rawptr);

  std::cout << "Number of bytes = " << VECTOR_SIZE << std::endl;

  auto start = shm::current_time();
  while (true) {
    msg->timestamp = shm::current_time();
    pub.publish(msg, VECTOR_SIZE);
    auto end = shm::current_time();
    auto diff = end - start;
    if (diff > (SECONDS + 1) * TIMESCALE_COUNT) break;
  }
  free(msg);
}

int main() {
  std::thread subscribe_thread(subscribe_loop);
  std::thread publish_thread(publish_loop);

  subscribe_thread.join();
  publish_thread.join();
}
