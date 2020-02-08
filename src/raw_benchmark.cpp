//
// Created by squadrick on 18/11/19.
//

#include <chrono>
#include <iostream>
#include <numeric>
#include <shadesmar/publisher.h>
#include <shadesmar/subscriber.h>

const std::string topic = "raw_benchmark_topic";
const int QUEUE_SIZE = 16;
const int SECONDS = 10;
const int VECTOR_SIZE = 10 * 1024 * 1024;

int count = 0, total_count = 0;
uint64_t lag = 0;

struct Message {
  uint64_t timestamp;
  void *data;
};

template <typename T> double get_mean(const std::vector<T> &v) {
  double sum = std::accumulate(v.begin(), v.end(), 0.0);
  double mean = sum / v.size();
  return mean;
}

template <typename T> double get_stddev(const std::vector<T> &v) {
  double mean = get_mean(v);
  std::vector<double> diff(v.size());
  std::transform(v.begin(), v.end(), diff.begin(),
                 [mean](double x) { return x - mean; });
  double sq_sum =
      std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);

  double stddev = std::sqrt(sq_sum / v.size());
  return stddev;
}

void callback(std::unique_ptr<uint8_t[]> &data, uint32_t size) {
  Message *msg = reinterpret_cast<Message *>(data.get());
  ++count;
  ++total_count;
  lag += std::chrono::duration_cast<TIMESCALE>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() -
         msg->timestamp;
}

int main() {
  if (fork() == 0) {
    std::vector<int> counts;
    std::vector<double> lags;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    shm::SubscriberBin<QUEUE_SIZE> sub(topic, callback);
    auto start = std::chrono::system_clock::now();
    int seconds = 0;
    while (true) {
      sub.spinOnce();
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<TIMESCALE>(end - start);

      if (diff.count() > TIMESCALE_COUNT) {
        double lag_per_msg = (double)lag / count;
        if (count != 0) {
          std::cout << "Number of msgs sent: " << count << "/s" << std::endl;
          std::cout << "Average Lag: " << lag_per_msg << TIMESCALE_NAME
                    << std::endl;

          counts.push_back(count);
          lags.push_back(lag_per_msg);
        } else {
          std::cout << "Number of message sent: <1/s" << std::endl;
        }

        if (++seconds == SECONDS)
          break;
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
    shm::PublisherBin<QUEUE_SIZE> pub(topic);

    Message *msg = (Message *)malloc(VECTOR_SIZE);

    std::cout << "Number of bytes = " << VECTOR_SIZE << std::endl;

    auto start = std::chrono::system_clock::now();
    while (true) {
      msg->timestamp = std::chrono::duration_cast<TIMESCALE>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
      pub.publish(msg, VECTOR_SIZE);
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<TIMESCALE>(end - start);
      if (diff.count() > (SECONDS + 2) * TIMESCALE_COUNT)
        break;
    }
    free(msg);
  }
}