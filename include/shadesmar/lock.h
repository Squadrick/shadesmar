//
// Created by squadrick on 19/12/19.
//

#ifndef SHADESMAR_LOCK_H
#define SHADESMAR_LOCK_H


template <typename LockT>
class ScopeGuard {
public:
  enum LockType { EXCLUSIVE, SHARED };
  ScopeGuard(LockT *lck, LockType type = EXCLUSIVE)
      : lck_(lck), type_(type) {
    if (type_ == SHARED) {
      lck_->lock_sharable();
    } else {
      lck_->lock();
    }
  }

  ~ScopeGuard() {
    if (type_ == SHARED) {
      lck_->unlock_sharable();
    } else {
      lck_->unlock();
    }
  }

private:
  LockT *lck_;
  LockType type_;
};

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

#endif // SHADESMAR_LOCK_H
