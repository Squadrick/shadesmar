//
// Created by squadrick on 18/01/20.
//

#include <shadesmar/macros.h>
#include <shadesmar/memory/memory.h>
#include <shadesmar/memory/tmp.h>

int main() {
  bool creat;
  auto *ptr = shm::create_memory_segment("test_shm", sizeof(int) * 100, creat);

  if (creat) {
    shm::tmp::write("test_shm");
    std::cout << "Writing" << std::endl;
    for (int i = 0; i < 100; ++i) {
      ptr[i] = i;
    }

    while (true)
      ;
  } else {
    std::cout << "Reading" << std::endl;
    for (int i = 0; i < 100; ++i)
      std::cout << ptr[i] << std::endl;
  }
}