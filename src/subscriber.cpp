//
// Created by squadrick on 8/4/19.
//

#include <shadesmar/subscriber.h>

#include <iostream>

void callback(std::shared_ptr<int> msg) {
  std::cout << *msg << std::endl;
}

int main() {
  shm::Subscriber<int> sub("test", callback);

  while (true) {
    sub.spinOnce();
  }
}