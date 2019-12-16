//
// Created by squadrick on 30/09/19.
//

#ifndef shadesmar_ROBUST_LOCK_H
#define shadesmar_ROBUST_LOCK_H

#ifdef __APPLE__
#define OLD_LOCK
#endif

#include <sys/stat.h>
#include <unistd.h>

#include <thread>

#include <shadesmar/lockless_set.h>
#include <shadesmar/macros.h>
#include <shadesmar/rw_lock.h>

namespace shm {

class RobustLock {
public:
  RobustLock();
  ~RobustLock();

  void lock();
  void unlock();
  void lock_sharable();
  void unlock_sharable();

private:
#ifdef OLD_LOCK
  void prune_sharable_procs();
  ReadWriteLock mutex_;
  std::atomic<__pid_t> exclusive_owner{0};
  LocklessSet shared_owners;
#else
  static void consistency_handler(pthread_mutex_t *mutex, int result);
  pthread_mutex_t r, g;
  pthread_mutexattr_t attr;
  uint32_t counter;
#endif
};

inline bool proc_dead(__pid_t proc) {
  if (proc == 0) {
    return false;
  }
  std::string pid_path = "/proc/" + std::to_string(proc);
  struct stat sts {};
  return (stat(pid_path.c_str(), &sts) == -1 && errno == ENOENT);
}

#ifdef OLD_LOCK

RobustLock::RobustLock() = default;

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
      prune_sharable_procs();
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

void RobustLock::prune_sharable_procs() {
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

#else

RobustLock::RobustLock() {
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

  pthread_mutex_init(&r, &attr);
  pthread_mutex_init(&g, &attr);
}

RobustLock::~RobustLock() {
  pthread_mutexattr_destroy(&attr);
  pthread_mutex_destroy(&r);
  pthread_mutex_destroy(&g);
}

void RobustLock::lock() {
  int res = pthread_mutex_lock(&g);
  consistency_handler(&g, res);
}

void RobustLock::unlock() { pthread_mutex_unlock(&g); }

void RobustLock::lock_sharable() {
  int res_r = pthread_mutex_lock(&r);
  consistency_handler(&r, res_r);
  counter += 1;
  if (counter == 1) {
    int res_g = pthread_mutex_lock(&g);
    consistency_handler(&g, res_g);
  }
  pthread_mutex_unlock(&r);
}

void RobustLock::unlock_sharable() {
  int res_r = pthread_mutex_lock(&r);
  consistency_handler(&r, res_r);
  counter -= 1;
  if (counter == 0) {
    pthread_mutex_unlock(&g);
  }
  pthread_mutex_unlock(&r);
}

void RobustLock::consistency_handler(pthread_mutex_t *mutex, int result) {
  if (result == EOWNERDEAD) {
    pthread_mutex_consistent(mutex);
  } else if (result == ENOTRECOVERABLE) {
    std::cerr << "Inconsistent mutex" << std::endl;
    exit(0);
  }
}

#endif

} // namespace shm
#endif // shadesmar_ROBUST_LOCK_H
