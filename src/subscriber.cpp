//
// Created by squadrick on 8/4/19.
//

#include <iostream>

#include <shadesmar/custom_msg.h>
#include <shadesmar/subscriber.h>

void callback(const std::shared_ptr<CustomMessage> &msg) {
  auto timestamp = std::chrono::duration_cast<TIMESCALE>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  std::cout << "Avg lag: " << timestamp - msg->timestamp << std::endl;
}

int main() {
  shm::Subscriber<CustomMessage, 16> sub("test", callback, true);

  while (true) {
    sub.spinOnce();
  }
}