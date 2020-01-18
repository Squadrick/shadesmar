//
// Created by squadrick on 18/01/20.
//

#include <shadesmar/macros.h>
#include <shadesmar/memory.h>
#include <shadesmar/tmp.h>

int main() {
  bool creat;
  auto *ptr = shm::create_memory_segment("test_shm", sizeof(int) * 100, creat);

  if (creat) {
    shm::tmp::write("test_shm");
    DEBUG("Writing");
    for (int i = 0; i < 100; ++i) {
      ptr[i] = i;
    }

    while(true);
  } else {
    DEBUG("Reading")
    for (int i = 0; i < 100; ++i)
      DEBUG(ptr[i]);
  }
}