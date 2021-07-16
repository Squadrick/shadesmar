/* MIT License

Copyright (c) 2020 Dheeraj R Reddy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

#ifndef INCLUDE_SHADESMAR_CONCURRENCY_LOCK_H_
#define INCLUDE_SHADESMAR_CONCURRENCY_LOCK_H_

#include <pthread.h>

#include <cerrno>
#include <iostream>

namespace shm::concurrent {

class PthreadWriteLock {
 public:
  PthreadWriteLock();
  ~PthreadWriteLock();

  void lock();
  bool try_lock();
  void unlock();
  void reset();

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
#endif  // __linux__
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
#endif  // __linux__
}

bool PthreadWriteLock::try_lock() { return pthread_mutex_trylock(&mutex); }

void PthreadWriteLock::unlock() { pthread_mutex_unlock(&mutex); }

void PthreadWriteLock::reset() {
  pthread_mutex_destroy(&mutex);
  pthread_mutex_init(&mutex, &attr);
}

}  // namespace shm::concurrent
#endif  // INCLUDE_SHADESMAR_CONCURRENCY_LOCK_H_
