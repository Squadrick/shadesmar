//
// Created by squadrick on 8/4/19.
//

#include <iostream>

#include <shadesmar/custom_msg.h>
#include <shadesmar/pubsub/subscriber.h>

void callback(const std::shared_ptr<CustomMessage> &msg) {
  std::cout << msg->seq << "\t";
  auto lag = std::chrono::duration_cast<TIMESCALE>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count() -
             msg->timestamp;
  std::cout << "Avg lag: " << lag << TIMESCALE_NAME << std::endl;

  for (auto i : msg->arr) {
    if (i != msg->val) {
      std::cerr << "Pubsub error" << std::endl;
      return;
    }
  }
}

int main() {
  shm::pubsub::Subscriber<CustomMessage, 16> sub("benchmark_topic", callback,
                                                 true);

  while (true) {
    sub.spin_once();
  }
}