//
// Created by squadrick on 8/4/19.
//

#include <iostream>

#include <boost/interprocess/shared_memory_object.hpp>

#include <shadesmar/tmp.h>

using namespace boost::interprocess;
void flush(std::string const &topic) {
  std::cout << "Flushing " << topic << std::endl;

  shared_memory_object::remove(topic.c_str());
  shared_memory_object::remove((topic + "Raw").c_str());
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