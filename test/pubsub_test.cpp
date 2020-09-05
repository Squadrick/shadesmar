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
#include <stdexcept>

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/message.h"
#include "shadesmar/pubsub/serialized_publisher.h"
#include "shadesmar/pubsub/serialized_subscriber.h"
#include "shadesmar/stats.h"
#endif

const char topic[] = "benchmark_topic";
const int SECONDS = 10;
const int VECTOR_SIZE = 1024 * 1024;

shm::stats::Welford per_second_lag;

class BenchmarkMsg : public shm::BaseMsg {
 public:
  uint8_t number{};
  std::vector<uint8_t> arr;
  SHM_PACK(number, arr);
  explicit BenchmarkMsg(uint8_t n) : number(n) {
    for (int i = 0; i < VECTOR_SIZE; ++i) arr.push_back(n);
  }
  BenchmarkMsg() = default;
};

void callback(const std::shared_ptr<BenchmarkMsg> &msg) {
  auto lag = static_cast<double>(shm::current_time() - msg->timestamp);
  per_second_lag.add(lag);

  for (auto i : msg->arr) {
    if (i != msg->number) {
      std::cerr << "Error on " << msg->number << std::endl;
      throw std::runtime_error("Subscriber data mismatch");
    }
  }
}

void subscribe_loop() {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  shm::pubsub::SerializedSubscriber<BenchmarkMsg> sub(topic, callback);
  int seconds = 0;

  auto start = shm::current_time();
  while (true) {
    sub.spin_once();
    auto end = shm::current_time();

    if (end - start > TIMESCALE_COUNT) {
      std::cout << "Lag (" << TIMESCALE_NAME << "): " << per_second_lag
                << std::endl;
      if (++seconds == SECONDS) break;
      start = shm::current_time();
      per_second_lag.clear();
    }
  }
}

void publish_loop() {
  shm::pubsub::SerializedPublisher<BenchmarkMsg> pub(topic, nullptr);

  msgpack::sbuffer buf;
  msgpack::pack(buf, BenchmarkMsg(VECTOR_SIZE));

  std::cout << "Number of bytes = " << buf.size() << std::endl;

  auto start = shm::current_time();

  int i = 0;
  while (true) {
    BenchmarkMsg msg(++i);
    msg.init_time();
    pub.publish(msg);
    auto end = shm::current_time();
    if (end - start > (SECONDS + 1) * TIMESCALE_COUNT) break;
  }
}

int main() {
  std::thread subscribe_thread(subscribe_loop);
  std::thread publish_thread(publish_loop);

  subscribe_thread.join();
  publish_thread.join();
}
