//
// Created by squadrick on 26/04/20.
//

#ifndef SHADESMAR_SCOPE_H
#define SHADESMAR_SCOPE_H

namespace shm::concurrent {

enum ExlOrShr { EXCLUSIVE, SHARED };

template <typename LockT, ExlOrShr type> class ScopeGuard;

template <typename LockT> class ScopeGuard<LockT, EXCLUSIVE> {
public:
  explicit ScopeGuard(LockT *lck) : lck_(lck) { lck_->lock(); }

  ~ScopeGuard() {
    lck_->unlock();
    lck_ = nullptr;
  }

private:
  LockT *lck_;
};

template <typename LockT> class ScopeGuard<LockT, SHARED> {
public:
  explicit ScopeGuard(LockT *lck) : lck_(lck) { lck_->lock_sharable(); }

  ~ScopeGuard() {
    lck_->unlock_sharable();
    lck_ = nullptr;
  }

private:
  LockT *lck_;
};
} // namespace shm::concurrent
#endif // SHADESMAR_SCOPE_H
