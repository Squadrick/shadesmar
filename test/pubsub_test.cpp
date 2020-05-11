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
#include <numeric>

#include "shadesmar/message.h"
#include "shadesmar/pubsub/publisher.h"
#include "shadesmar/pubsub/subscriber.h"

const char topic[] = "benchmark_topic";
const int QUEUE_SIZE = 16;
const int SECONDS = 10;
const int VECTOR_SIZE = 10 * 1024 * 1024;
const bool EXTRA_COPY = false;

int count = 0, total_count = 0;
uint64_t lag = 0;

class BenchmarkMsg : public shm::BaseMsg {
 public:
  int number{};
  std::vector<uint8_t> arr;
  SHM_PACK(number, arr);
  explicit BenchmarkMsg(int n) : number(n) {
    for (int i = 0; i < VECTOR_SIZE; ++i) arr.push_back(n);
  }
  BenchmarkMsg() = default;
};

template <typename T>
double get_mean(const std::vector<T> &v) {
  double sum = std::accumulate(v.begin(), v.end(), 0.0);
  double mean = sum / v.size();
  return mean;
}

template <typename T>
double get_stddev(const std::vector<T> &v) {
  double mean = get_mean(v);
  std::vector<double> diff(v.size());
  std::transform(v.begin(), v.end(), diff.begin(),
                 [mean](double x) { return x - mean; });
  double sq_sum =
      std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);

  double stddev = std::sqrt(sq_sum / v.size());
  return stddev;
}

void callback(const std::shared_ptr<BenchmarkMsg> &msg) {
  ++count;
  ++total_count;
  lag += std::chrono::duration_cast<TIMESCALE>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() -
         msg->timestamp;

  for (auto i : msg->arr) {
    if (i != msg->number) {
      std::cerr << "Error on " << msg->number << std::endl;
      exit(1);
    }
  }
}

int main() {
  if (fork() == 0) {
    std::vector<int> counts;
    std::vector<double> lags;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    shm::pubsub::Subscriber<BenchmarkMsg, QUEUE_SIZE> sub(topic, callback,
                                                          EXTRA_COPY);
    auto start = std::chrono::system_clock::now();
    int seconds = 0;
    while (true) {
      sub.spin_once();
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<TIMESCALE>(end - start);

      if (diff.count() > TIMESCALE_COUNT) {
        double lag_per_msg = static_cast<double>(lag) / count;
        if (count != 0) {
          std::cout << "Number of msgs sent: " << count << "/s" << std::endl;
          std::cout << "Average Lag: " << lag_per_msg << TIMESCALE_NAME
                    << std::endl;
          counts.push_back(count);
          lags.push_back(lag_per_msg);
        } else {
          std::cout << "Number of message sent: <1/s" << std::endl;
        }

        if (++seconds == SECONDS) break;
        count = 0;
        lag = 0;
        start = std::chrono::system_clock::now();
      }
    }

    std::cout << "Total msgs sent in 10 seconds: " << total_count << std::endl;

    auto mean_count = get_mean(counts);
    auto stdd_count = get_stddev(counts);
    std::cout << "Msg: " << mean_count << " ± " << stdd_count << std::endl;

    auto mean_lag = get_mean(lags);
    auto stdd_lag = get_stddev(lags);
    std::cout << "Lag: " << mean_lag << " ± " << stdd_lag << TIMESCALE_NAME
              << std::endl;

  } else {
    shm::pubsub::Publisher<BenchmarkMsg, QUEUE_SIZE> pub(topic);

    msgpack::sbuffer buf;
    msgpack::pack(buf, BenchmarkMsg(VECTOR_SIZE));

    std::cout << "Number of bytes = " << buf.size() << std::endl;

    auto start = std::chrono::system_clock::now();

    int i = 0;
    while (true) {
      BenchmarkMsg msg(++i);
      msg.init_time();
      pub.publish(msg);
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<TIMESCALE>(end - start);
      if (diff.count() > (SECONDS + 1) * TIMESCALE_COUNT) break;
    }
  }
}
