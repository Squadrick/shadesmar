//
// Created by squadrick on 09/10/19.
//

#include <shadesmar/memory.h>

int main(int argc, char **argv) {
  auto topic = std::string(argv[1]);
  shm::Memory ls(topic, false);
  ls.dump();
}