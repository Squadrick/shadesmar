//
// Created by squadrick on 11/12/19.
//

#ifndef SHADESMAR_RW_LOCK_H
#define SHADESMAR_RW_LOCK_H

#include <pthread.h>
#include <shadesmar/lock.h>
#include <shadesmar/macros.h>

namespace shm {

class ReadWriteLock {
public:
  ReadWriteLock();
  ~ReadWriteLock();

  void lock();
  bool try_lock();
  void unlock();
  void lock_sharable();
  bool try_lock_sharable();
  void unlock_sharable();

private:
  pthread_mutex_t mut;
  pthread_mutexattr_t mutex_attr;

  pthread_cond_t read_condvar, write_condvar;
  pthread_condattr_t condvar_attr;

  int rw_count;
  bool read_waiting;
};

ReadWriteLock::ReadWriteLock() {
  rw_count = 0;
  read_waiting = false;

  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&mut, &mutex_attr);

  pthread_condattr_init(&condvar_attr);
  pthread_condattr_setpshared(&condvar_attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&read_condvar, &condvar_attr);
  pthread_cond_init(&write_condvar, &condvar_attr);
}

ReadWriteLock::~ReadWriteLock() {
  pthread_mutexattr_destroy(&mutex_attr);
  pthread_mutex_destroy(&mut);
  pthread_condattr_destroy(&condvar_attr);
  pthread_cond_destroy(&read_condvar);
  pthread_cond_destroy(&write_condvar);
}

void ReadWriteLock::lock() {
  PthreadGuard scope(&mut);

  while (rw_count != 0) {
    pthread_cond_wait(&write_condvar, &mut);
  }

  rw_count = -1;
}

bool ReadWriteLock::try_lock() {
  PthreadGuard scope(&mut, TRY_LOCK);

  if (!scope.owns() || rw_count != 0) {
    return false;
  }

  rw_count = -1;
  return true;
}

void ReadWriteLock::unlock() {
  PthreadGuard scope(&mut);
  rw_count = 0;

  if (read_waiting) {
    pthread_cond_broadcast(&read_condvar);
  } else {
    pthread_cond_signal(&write_condvar);
  }
}

void ReadWriteLock::lock_sharable() {
  PthreadGuard scope(&mut);
  read_waiting = true;

  while (rw_count < 0) {
    pthread_cond_wait(&read_condvar, &mut);
  }
  read_waiting = false;
  ++rw_count;
}

bool ReadWriteLock::try_lock_sharable() {
  PthreadGuard scope(&mut, TRY_LOCK);

  if (!scope.owns() || rw_count < 0) {
    return false;
  }
  read_waiting = false;
  ++rw_count;

  return true;
}

void ReadWriteLock::unlock_sharable() {
  PthreadGuard scope(&mut);
  --rw_count;

  if (rw_count == 0) {
    pthread_cond_signal(&write_condvar);
  }
}

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
  pthread_rwlock_t rwlock;
  pthread_rwlockattr_t attr;
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
