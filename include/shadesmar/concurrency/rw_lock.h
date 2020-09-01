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

#ifndef INCLUDE_SHADESMAR_CONCURRENCY_RW_LOCK_H_
#define INCLUDE_SHADESMAR_CONCURRENCY_RW_LOCK_H_

#include <pthread.h>

#include "shadesmar/concurrency/lock.h"
#include "shadesmar/macros.h"

namespace shm::concurrent {

class PthreadReadWriteLock {
 public:
  PthreadReadWriteLock();
  ~PthreadReadWriteLock();

  void lock();
  bool try_lock();
  void unlock();
  void lock_sharable();
  bool try_lock_sharable();
  void unlock_sharable();
  void reset();

 private:
  pthread_rwlock_t rwlock{};
  pthread_rwlockattr_t attr{};
};

PthreadReadWriteLock::PthreadReadWriteLock() {
  pthread_rwlockattr_init(&attr);
  pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_rwlock_init(&rwlock, &attr);
}
PthreadReadWriteLock::~PthreadReadWriteLock() {
  pthread_rwlockattr_destroy(&attr);
  pthread_rwlock_destroy(&rwlock);
}

void PthreadReadWriteLock::lock() { pthread_rwlock_wrlock(&rwlock); }
bool PthreadReadWriteLock::try_lock() {
  return (!pthread_rwlock_trywrlock(&rwlock));
}
void PthreadReadWriteLock::unlock() { pthread_rwlock_unlock(&rwlock); }
void PthreadReadWriteLock::lock_sharable() { pthread_rwlock_rdlock(&rwlock); }
bool PthreadReadWriteLock::try_lock_sharable() {
  return (!pthread_rwlock_tryrdlock(&rwlock));
}
void PthreadReadWriteLock::unlock_sharable() {
  pthread_rwlock_unlock(&rwlock);
}

void PthreadReadWriteLock::reset() {
  pthread_rwlock_destroy(&rwlock);
  pthread_rwlock_init(&rwlock, &attr);
}

}  // namespace shm::concurrent
#endif  // INCLUDE_SHADESMAR_CONCURRENCY_RW_LOCK_H_
