//
// Created by squadrick on 8/4/19.
//

#include <shadesmar/memory.h>

int main(int argc, char **argv) {
  std::string topic(argv[1]);
  std::cout << "Flushing " << topic << std::endl;
  shm::Memory mem(topic, 0, false);
  mem.remove_old_shared_memory();

  boost::interprocess::named_mutex::remove((topic + "MemMutex").c_str());
  boost::interprocess::named_mutex::remove((topic + "InfoMutex").c_str());
}