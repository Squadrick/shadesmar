//
// Created by squadrick on 8/4/19.
//

#include <sys/mman.h>

#include <iostream>

#include <shadesmar/tmp.h>

void flush(std::string const &topic) {
  std::cout << "Flushing " << topic << std::endl;
  shm_unlink(topic.c_str());
}

int main(int argc, char **argv) {
  if (argc == 1) {
    for (auto &topic : shm::tmp::get_topics()) {
      flush(topic);
    }
    shm::tmp::delete_topics();
  } else {
    flush(std::string(argv[1]));
  }
}