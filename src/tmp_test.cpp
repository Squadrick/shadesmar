//
// Created by squadrick on 10/09/19.
//

#include <iostream>

#include <shadesmar/tmp.h>

int main() {
  for (int i = 0; i < 10; ++i) {
    shm::tmp::write_topic(shm::tmp::random_string(10));
  }

  for (const auto &topic : shm::tmp::get_topics()) {
    std::cout << topic << std::endl;
  }

  shm::tmp::delete_topics();
}