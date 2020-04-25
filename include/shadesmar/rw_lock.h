//
// Created by squadrick on 11/12/19.
//

#ifndef SHADESMAR_RW_LOCK_H
#define SHADESMAR_RW_LOCK_H

#include <pthread.h>
#include <shadesmar/lock.h>
#include <shadesmar/macros.h>

namespace shm {

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
void PthreadReadWriteLock::unlock_sharable() { pthread_rwlock_unlock(&rwlock); }

} // namespace shm
#endif // SHADESMAR_RW_LOCK_H
