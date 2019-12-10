//
// Created by squadrick on 11/12/19.
//

#ifndef SHADESMAR_RW_LOCK_H
#define SHADESMAR_RW_LOCK_H

#include <pthread.h>
#include <shadesmar/macros.h>

class MutexGuard {

public:
  enum LockType { LOCK, TRY_LOCK };
  MutexGuard(pthread_mutex_t *mut, LockType type = LOCK);
  ~MutexGuard();

  bool owns();

private:
  pthread_mutex_t *_mut;
  int res;
};

MutexGuard::MutexGuard(pthread_mutex_t *mut, LockType type) {
  if (type == LOCK) {
    res = pthread_mutex_lock(mut);
  } else if (type == TRY_LOCK) {
    res = pthread_mutex_trylock(mut);
  }
  _mut = mut;
}

MutexGuard::~MutexGuard() {
  if (res == 0) {
    pthread_mutex_unlock(_mut);
  }
  _mut = nullptr;
}

bool MutexGuard::owns() { return res == 0; }

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
  pthread_mutex_lock(&mut);

  while (rw_count != 0) {
    pthread_cond_wait(&write_condvar, &mut);
  }

  rw_count = -1;
  pthread_mutex_unlock(&mut);
}

bool ReadWriteLock::try_lock() {
  MutexGuard scope(&mut, MutexGuard::LOCK);
  if (!scope.owns() || rw_count != 0) {
    return false;
  }

  rw_count = -1;
  pthread_mutex_unlock(&mut);

  return true;
}

void ReadWriteLock::unlock() {
  pthread_mutex_lock(&mut);

  rw_count = 0;

  if (read_waiting) {
    pthread_cond_broadcast(&read_condvar);
  } else {
    pthread_cond_signal(&write_condvar);
  }

  pthread_mutex_unlock(&mut);
}

void ReadWriteLock::lock_sharable() {
  pthread_mutex_lock(&mut);
  read_waiting = true;

  while (rw_count < 0) {
    pthread_cond_wait(&read_condvar, &mut);
  }
  read_waiting = false;
  ++rw_count;
  pthread_mutex_unlock(&mut);
}

bool ReadWriteLock::try_lock_sharable() {
  MutexGuard scope(&mut, MutexGuard::TRY_LOCK);

  if (!scope.owns() || rw_count < 0) {
    return false;
  }
  read_waiting = false;
  ++rw_count;
  pthread_mutex_unlock(&mut);

  return true;
}

void ReadWriteLock::unlock_sharable() {
  pthread_mutex_lock(&mut);
  --rw_count;

  if (rw_count == 0) {
    pthread_cond_signal(&write_condvar);
  }

  pthread_mutex_unlock(&mut);
}

#endif // SHADESMAR_RW_LOCK_H
