//
// Created by squadrick on 30/09/19.
//

#ifndef shadesmar_ROBUST_LOCK_H
#define shadesmar_ROBUST_LOCK_H

#include <sys/stat.h>
#include <unistd.h>

#include <thread>

#include <shadesmar/concurrency/lock.h>
#include <shadesmar/concurrency/lockless_set.h>
#include <shadesmar/concurrency/rw_lock.h>
#include <shadesmar/macros.h>

namespace shm {

class RobustLock {
public:
  RobustLock();
  RobustLock(const RobustLock &);
  ~RobustLock();

  void lock();
  void unlock();
  void lock_sharable();
  void unlock_sharable();

private:
  void prune_readers();
  PthreadReadWriteLock mutex_;
  std::atomic<__pid_t> exclusive_owner{0};
  LocklessSet shared_owners;
};

inline bool proc_dead(__pid_t proc) {
  if (proc == 0) {
    return false;
  }
  std::string pid_path = "/proc/" + std::to_string(proc);
  struct stat sts {};
  return (stat(pid_path.c_str(), &sts) == -1 && errno == ENOENT);
}

RobustLock::RobustLock() = default;

RobustLock::RobustLock(const RobustLock &lock) {
  mutex_ = lock.mutex_;
  exclusive_owner.store(lock.exclusive_owner.load());
  shared_owners = lock.shared_owners;
}

RobustLock::~RobustLock() { exclusive_owner = 0; }

void RobustLock::lock() {
  while (!mutex_.try_lock()) {
    if (exclusive_owner != 0) {
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
  while (!shared_owners.insert(getpid()))
    ;
}

void RobustLock::unlock_sharable() {
  if (shared_owners.remove(getpid())) {
    mutex_.unlock_sharable();
  }
}

void RobustLock::prune_readers() {
  for (auto &i : shared_owners.__array) {
    uint32_t shared_owner = i.load();

    if (shared_owner == 0)
      continue;
    if (proc_dead(shared_owner)) {
      if (shared_owners.remove(shared_owner)) {
        mutex_.unlock_sharable();
      }
    }
  }
}

} // namespace shm
#endif // shadesmar_ROBUST_LOCK_H
