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

#ifndef INCLUDE_SHADESMAR_CONCURRENCY_SCOPE_H_
#define INCLUDE_SHADESMAR_CONCURRENCY_SCOPE_H_

namespace shm::concurrent {

enum ExlOrShr { EXCLUSIVE, SHARED };

template <typename LockT, ExlOrShr type>
class ScopeGuard;

template <typename LockT>
class ScopeGuard<LockT, EXCLUSIVE> {
 public:
  explicit ScopeGuard(LockT *lck) : lck_(lck) {
    if (lck_ != nullptr) {
      lck_->lock();
    }
  }

  ~ScopeGuard() {
    if (lck_ != nullptr) {
      lck_->unlock();
      lck_ = nullptr;
    }
  }

 private:
  LockT *lck_;
};

template <typename LockT>
class ScopeGuard<LockT, SHARED> {
 public:
  explicit ScopeGuard(LockT *lck) : lck_(lck) { lck_->lock_sharable(); }

  ~ScopeGuard() {
    if (lck_ != nullptr) {
      lck_->unlock_sharable();
      lck_ = nullptr;
    }
  }

 private:
  LockT *lck_;
};
}  // namespace shm::concurrent
#endif  // INCLUDE_SHADESMAR_CONCURRENCY_SCOPE_H_
