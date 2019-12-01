//
// Created by squadrick on 02/12/19.
//

#include <iostream>

#include <shadesmar/ipc_lock.h>
#include <shadesmar/memory.h>
#include <shadesmar/tmp.h>

typedef shm::IPC_Lock LOCK;

int main() {
  bool new_segment;
  uint8_t *buff =
      shm::create_memory_segment("lock_buffer", sizeof(LOCK), new_segment);

  LOCK *lock;
  if (new_segment) {
    std::cout << "Creating new lock" << std::endl;
    lock = new (buff) LOCK;
    shm::tmp::write_topic("lock_buffer");
  } else {
    lock = reinterpret_cast<LOCK *>(buff);
  }

  std::cout << "WL: Writer lock" << std::endl;
  std::cout << "WU: Writer unlock" << std::endl;
  std::cout << "RL: Reader lock" << std::endl;
  std::cout << "RU: Reader unlock" << std::endl;
  while (true) {
    std::cout << "Input: ";
    std::string ln;
    std::cin >> ln;

    if (ln == "WL") {
      lock->lock();
    } else if (ln == "WU") {
      lock->unlock();
    } else if (ln == "RL") {
      lock->lock_sharable();
    } else if (ln == "RU") {
      lock->unlock_sharable();
    } else {
      break;
    }

    std::cout << "Success" << std::endl;
  }
}