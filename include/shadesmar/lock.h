//
// Created by squadrick on 19/12/19.
//

#ifndef SHADESMAR_LOCK_H
#define SHADESMAR_LOCK_H

class LockInterface {
public:
  virtual void lock() = 0;
  virtual bool try_lock() = 0;
  virtual void unlock() = 0;
  virtual void lock_sharable() = 0;
  virtual bool try_lock_sharable() = 0;
  virtual void unlock_sharable() = 0;
};

class ScopeGuard {
public:
  enum LockType { EXCLUSIVE, SHARED };
  ScopeGuard(LockInterface *lck, LockType type = EXCLUSIVE)
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
  LockInterface *lck_;
  LockType type_;
};

#endif // SHADESMAR_LOCK_H
