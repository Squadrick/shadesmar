//
// Created by squadrick on 8/4/19.
//

#include <shadesmar/memory.h>
#include <shadesmar/tmp.h>

void flush(std::string const &topic) {
  std::cout << "Flushing " << topic << std::endl;

  shm::Memory info(topic, 0, false);
  info.remove_old_shared_memory();

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