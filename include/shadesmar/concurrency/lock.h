//
// Created by squadrick on 19/12/19.
//

#ifndef SHADESMAR_LOCK_H
#define SHADESMAR_LOCK_H

#include <pthread.h>

namespace shm {

class PthreadWriteLock {
public:
  PthreadWriteLock();
  ~PthreadWriteLock();

  void lock();
  bool try_lock();
  void unlock();

  pthread_mutex_t *get_mutex() { return &mutex; }

private:
  pthread_mutex_t mutex{};
  pthread_mutexattr_t attr{};
};

PthreadWriteLock::PthreadWriteLock() {
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
#ifdef __linux__
  pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
#endif
  pthread_mutex_init(&mutex, &attr);
}

PthreadWriteLock::~PthreadWriteLock() {
  pthread_mutexattr_destroy(&attr);
  pthread_mutex_destroy(&mutex);
}

void PthreadWriteLock::lock() {
  pthread_mutex_lock(&mutex);
#ifdef __linux__
  if (errno == EOWNERDEAD) {
    std::cerr << "Previous owner of mutex was dead." << std::endl;
  }
#endif
}

bool PthreadWriteLock::try_lock() { return pthread_mutex_trylock(&mutex); }

void PthreadWriteLock::unlock() { pthread_mutex_unlock(&mutex); }

} // namespace shm
#endif // SHADESMAR_LOCK_H
