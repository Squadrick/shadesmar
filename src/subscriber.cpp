//
// Created by squadrick on 8/4/19.
//

#include <iostream>

#include <shadesmar/custom_msg.h>
#include <shadesmar/subscriber.h>

void callback(const std::shared_ptr<CustomMessage> &msg) {
  DEBUG_IMPL(msg->seq, "\t");
  auto lag = std::chrono::duration_cast<TIMESCALE>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count() -
             msg->timestamp;
  DEBUG("Avg lag: " << lag << TIMESCALE_NAME);

  for (auto i : msg->arr) {
    if (i != msg->val) {
      DEBUG("Pubsub error");
      return;
    }
  }
}

int main() {
  shm::Subscriber<CustomMessage, 16> sub("test_ben1", callback, true);

  while (true) {
    sub.spinOnce();
  }
}