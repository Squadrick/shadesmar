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

#ifndef INCLUDE_SHADESMAR_CONCURRENCY_ROBUST_LOCK_H_
#define INCLUDE_SHADESMAR_CONCURRENCY_ROBUST_LOCK_H_

#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <string>
#include <thread>

#include "shadesmar/concurrency/lock.h"
#include "shadesmar/concurrency/lockless_set.h"
#include "shadesmar/concurrency/rw_lock.h"
#include "shadesmar/macros.h"

namespace shm::concurrent {

class RobustLock {
 public:
  RobustLock();
  RobustLock(const RobustLock &);
  ~RobustLock();

  void lock();
  bool try_lock();
  void unlock();
  void lock_sharable();
  bool try_lock_sharable();
  void unlock_sharable();
  void reset();

 private:
  void prune_readers();
  PthreadReadWriteLock mutex_;
  std::atomic<__pid_t> exclusive_owner{0};
  LocklessSet<8> shared_owners;
};

RobustLock::RobustLock() = default;

RobustLock::RobustLock(const RobustLock &lock) {
  mutex_ = lock.mutex_;
  exclusive_owner.store(lock.exclusive_owner.load());
  shared_owners = lock.shared_owners;
}

RobustLock::~RobustLock() { exclusive_owner = 0; }

void RobustLock::lock() {
  while (!mutex_.try_lock()) {
    if (exclusive_owner.load() != 0) {
      auto ex_proc = exclusive_owner.load();
      if (proc_dead(ex_proc)) {
        if (exclusive_owner.compare_exchange_strong(ex_proc, 0)) {
          mutex_.unlock();
          continue;
        }
      }
    } else {
      prune_readers();
    }

    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
  exclusive_owner = getpid();
}

bool RobustLock::try_lock() {
  if (!mutex_.try_lock()) {
    if (exclusive_owner != 0) {
      auto ex_proc = exclusive_owner.load();
      if (proc_dead(ex_proc)) {
        if (exclusive_owner.compare_exchange_strong(ex_proc, 0)) {
          mutex_.unlock();
        }
      }
    } else {
      prune_readers();
    }
    if (mutex_.try_lock()) {
      exclusive_owner = getpid();
      return true;
    } else {
      return false;
    }
  } else {
    exclusive_owner = getpid();
    return true;
  }
}

void RobustLock::unlock() {
  __pid_t current_pid = getpid();
  if (exclusive_owner.compare_exchange_strong(current_pid, 0)) {
    mutex_.unlock();
  }
}

void RobustLock::lock_sharable() {
  while (!mutex_.try_lock_sharable()) {
    if (exclusive_owner != 0) {
      auto ex_proc = exclusive_owner.load();
      if (proc_dead(ex_proc)) {
        if (exclusive_owner.compare_exchange_strong(ex_proc, 0)) {
          exclusive_owner = 0;
          mutex_.unlock();
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
  while (!shared_owners.insert(getpid())) {
  }
}

bool RobustLock::try_lock_sharable() {
  if (!mutex_.try_lock_sharable()) {
    if (exclusive_owner != 0) {
      auto ex_proc = exclusive_owner.load();
      if (proc_dead(ex_proc)) {
        if (exclusive_owner.compare_exchange_strong(ex_proc, 0)) {
          exclusive_owner = 0;
          mutex_.unlock();
        }
      }
    }
    if (mutex_.try_lock_sharable()) {
      while (!shared_owners.insert(getpid())) {
      }
      return true;
    } else {
      return false;
    }
  } else {
    while (!shared_owners.insert(getpid())) {
    }
    return true;
  }
}

void RobustLock::unlock_sharable() {
  if (shared_owners.remove(getpid())) {
    mutex_.unlock_sharable();
  }
}

void RobustLock::reset() { mutex_.reset(); }

void RobustLock::prune_readers() {
  for (auto &i : shared_owners.array_) {
    uint32_t shared_owner = i.load();

    if (shared_owner == 0) continue;
    if (proc_dead(shared_owner)) {
      if (shared_owners.remove(shared_owner)) {
        mutex_.unlock_sharable();
      }
    }
  }
}

}  // namespace shm::concurrent
#endif  // INCLUDE_SHADESMAR_CONCURRENCY_ROBUST_LOCK_H_
