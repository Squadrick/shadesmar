//
// Created by squadrick on 10/09/19.
//

#include <iostream>

#include <shadesmar/memory/tmp.h>

int main() {
  for (int i = 0; i < 10; ++i) {
    shm::memory::tmp::write(shm::memory::tmp::random_string(10));
  }

  for (const auto &topic : shm::memory::tmp::get_tmp_names()) {
    std::cout << topic << std::endl;
  }

  shm::memory::tmp::delete_topics();
}