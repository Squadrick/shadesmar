//
// Created by squadrick on 8/4/19.
//

#include <iostream>

#include <shadesmar/message.h>
#include <shadesmar/subscriber.h>

void callback(const std::shared_ptr<shm::Msg<MSG_SIZE>> &msg) {
  int val = msg->count;

  std::cout << val << std::endl;

  for (unsigned char byte : msg->bytes) {
    if (byte != val) {
      std::cerr << "Error at value: " << val << "->" << (int)byte << std::endl;
      break;
    }
  }
}

int main() {
  shm::Subscriber<shm::Msg<MSG_SIZE>, 16> sub("test", callback);

  while (true) {
    sub.spinOnce();
  }
}