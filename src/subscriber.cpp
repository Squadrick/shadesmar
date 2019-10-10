//
// Created by squadrick on 8/4/19.
//

#include <iostream>

#include <shadesmar/custom_msg.h>
#include <shadesmar/subscriber.h>

void callback(const std::shared_ptr<CustomMessage> &msg) {
  std::cout << msg->seq << std::endl;
  std::cout << msg->frame_id << std::endl;
  std::cout << msg->timestamp << std::endl;
  std::cout << msg->val << std::endl;
  std::cout << std::endl;
}

int main() {
  shm::Subscriber<CustomMessage, 16> sub("test", callback, true);

  while (true) {
    sub.spinOnce();
  }
}