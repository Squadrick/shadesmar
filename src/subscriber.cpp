//
// Created by squadrick on 8/4/19.
//

#include <shadesmar/message.h>
#include <shadesmar/subscriber.h>

#include <unistd.h>

#include <iostream>

void callback(std::shared_ptr<shm::Msg> msg) {
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
  shm::Subscriber<shm::Msg> sub("test", callback);

  while (true) {
    sub.spinOnce();
  }
}