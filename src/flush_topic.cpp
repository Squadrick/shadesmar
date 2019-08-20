//
// Created by squadrick on 8/4/19.
//

#include <shadesmar/memory.h>

int main(int argc, char **argv) {
  std::string topic(argv[1]);
  std::cout << "Flushing " << topic << std::endl;

  std::string mem_name = topic + "Mem";
  shm::Memory mem(mem_name, 0, false);
  mem.remove_old_shared_memory();

  std::string info_name = topic + "Info";
  shm::Memory info(info_name, 0, false);
  info.remove_old_shared_memory();

  boost::interprocess::named_mutex::remove((topic + "MemMutex").c_str());
  boost::interprocess::named_mutex::remove((topic + "InfoMutex").c_str());
}