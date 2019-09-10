//
// Created by squadrick on 8/4/19.
//

#include <shadesmar/memory.h>
#include <shadesmar/tmp.h>

void flush(std::string const &topic) {
  std::cout << "Flushing " << topic << std::endl;

  std::string mem_name = topic + "Mem";
  shm::Memory mem(mem_name, 0, false);
  mem.remove_old_shared_memory();

  std::string info_name = topic + "Info";
  shm::Memory info(info_name, 0, false);
  info.remove_old_shared_memory();
}

int main(int argc, char **argv) {
  if (argc == 1) {
    for(auto& topic: shm::tmp_get_topics()) {
      flush(topic);
    }
  }
  else {
    flush(std::string(argv[1]));
  }
}