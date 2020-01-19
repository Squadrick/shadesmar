//
// Created by squadrick on 18/11/19.
//

#include <chrono>
#include <iostream>
#include <shadesmar/publisher.h>
#include <shadesmar/subscriber.h>

const int QUEUE_SIZE = 16;
const int SECONDS = 10;
const int VECTOR_SIZE = 10 * 1024 * 1024;

int count = 0;
uint64_t lag = 0;

struct Message {
  uint64_t timestamp;
  void *data;
};

void callback(std::unique_ptr<uint8_t[]> &data, uint32_t size) {
  Message *msg = reinterpret_cast<Message *>(data.get());
  ++count;
  lag += std::chrono::duration_cast<TIMESCALE>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() -
         msg->timestamp;
}

int main() {
  if (fork() == 0) {
    sleep(1);
    shm::SubscriberBin<QUEUE_SIZE> sub("benchmark_bin", callback);
    auto start = std::chrono::system_clock::now();
    int seconds = 0;
    while (true) {
      sub.spinOnce();
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<TIMESCALE>(end - start);

      if (diff.count() > TIMESCALE_COUNT) {
        double lag_ = (double)lag / count;
        std::cout << "Number of msgs sent: " << count << "/s" << std::endl;
        std::cout << "Average Lag: " << lag_ << TIMESCALE_NAME << std::endl;

        if (++seconds == SECONDS)
          break;
        count = 0;
        lag = 0;
        start = std::chrono::system_clock::now();
      }
    }
  } else {
    shm::PublisherBin<QUEUE_SIZE> pub("benchmark_bin");
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
      //      std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    free(msg);
  }
}