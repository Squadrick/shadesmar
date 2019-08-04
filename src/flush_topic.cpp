//
// Created by squadrick on 8/4/19.
//

#include <shadesmar/memory.h>

int main(int argc, char **argv) {
  std::cout << "Flushing " << argv[1] << std::endl;
  shm::Memory mem(argv[1], 0, false);
  mem.remove_old_shared_memory();
}