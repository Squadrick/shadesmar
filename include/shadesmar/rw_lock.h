//
// Created by squadrick on 11/12/19.
//

#ifndef SHADESMAR_RW_LOCK_H
#define SHADESMAR_RW_LOCK_H

#include <pthread.h>
#include <shadesmar/lock.h>
#include <shadesmar/macros.h>

class PthreadGuard {

public:
  enum LockType { LOCK, TRY_LOCK };
  PthreadGuard(pthread_mutex_t *mut, LockType type = LOCK);
  ~PthreadGuard();

  bool owns();

private:
  pthread_mutex_t *mut_;
  int res;
};

PthreadGuard::PthreadGuard(pthread_mutex_t *mut, LockType type) : mut_(mut) {
  if (type == LOCK) {
    res = pthread_mutex_lock(mut);
  } else if (type == TRY_LOCK) {
    res = pthread_mutex_trylock(mut);
  }
}

PthreadGuard::~PthreadGuard() {
  if (res == 0) {
    pthread_mutex_unlock(mut_);
  }
  mut_ = nullptr;
}

bool PthreadGuard::owns() { return res == 0; }

class ReadWriteLock : public LockInterface {
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
  PthreadGuard scope(&mut, PthreadGuard::TRY_LOCK);

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
  PthreadGuard scope(&mut, PthreadGuard::TRY_LOCK);

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

#endif // SHADESMAR_RW_LOCK_H
