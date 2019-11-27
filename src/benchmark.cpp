//
// Created by squadrick on 11/10/19.
//
#include <iostream>
#include <shadesmar/message.h>
#include <shadesmar/publisher.h>
#include <shadesmar/subscriber.h>

const std::string topic = "bench";

const int QUEUE_SIZE = 16;
const int SECONDS = 10;
const int VECTOR_SIZE = 10 * 1024 * 1024;
const bool EXTRA_COPY = false;

class BenchmarkMsg : public shm::BaseMsg {
public:
  std::vector<uint8_t> arr;
  SHM_PACK(arr);
  explicit BenchmarkMsg(int n) {
    for (int i = 0; i < n; ++i)
      arr.push_back(255);
  }

  BenchmarkMsg() = default;
};

int count = 0;
uint64_t lag = 0;
void callback(const std::shared_ptr<BenchmarkMsg> &msg) {
  ++count;
  lag += std::chrono::duration_cast<TIMESCALE>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() -
         msg->timestamp;
}

int main() {
  if (fork() == 0) {
    sleep(1);
    shm::Subscriber<BenchmarkMsg, QUEUE_SIZE> sub(topic, callback, EXTRA_COPY);
    auto start = std::chrono::system_clock::now();
    int seconds = 0;
    while (true) {
      sub.spinOnce();
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<TIMESCALE>(end - start);

      if (diff.count() > TIMESCALE_COUNT) {
        double lag_ = (double)lag / count;
        DEBUG("Number of messages sent: " << count << "/s");
        DEBUG("Average Lag: " << lag_ << TIMESCALE_NAME);

        if (++seconds == SECONDS)
          break;
        count = 0;
        lag = 0;
        start = std::chrono::system_clock::now();
      }
    }
  } else {
    shm::Publisher<BenchmarkMsg, QUEUE_SIZE> pub(topic);
    BenchmarkMsg msg(VECTOR_SIZE);
    msgpack::sbuffer buf;
    msgpack::pack(buf, msg);
    DEBUG("Number of bytes = " << buf.size());
    auto start = std::chrono::system_clock::now();

    while (true) {
      msg.init_time();
      pub.publish(msg);
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<TIMESCALE>(end - start);
      if (diff.count() > (SECONDS + 2) * TIMESCALE_COUNT)
        break;
    }
  }
}
