//
// Created by squadrick on 10/09/19.
//

#include <iostream>

#include <shadesmar/tmp.h>

int main() {
  for (int i = 0; i < 10; ++i) shm::tmp_write_topic(shm::random_string(10));

  for (auto topic : shm::tmp_get_topics()) std::cout << topic << std::endl;
}