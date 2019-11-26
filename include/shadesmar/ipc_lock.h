//
// Created by squadrick on 30/09/19.
//

#ifndef shadesmar_IPC_LOCK_H
#define shadesmar_IPC_LOCK_H

#ifdef __APPLE__
  #define __pid_t __darwin_pid_t
#endif

#include <sys/stat.h>

#include <cstdint>
#include <thread>

#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>

#include <shadesmar/macros.h>

using namespace boost::interprocess;

const int MAX_SHARED_OWNERS = 16;

namespace shm {

template <uint32_t size> class IPC_Set {
public:
  IPC_Set() { std::memset(__array, 0, size); }

  bool insert(uint32_t elem) {
    for (uint32_t idx = 0; idx < size; ++idx) {
      auto probedElem = __array[idx].load();

      if (probedElem != elem) {
        // The entry is either free or contains another key
        if (probedElem != 0) {
          continue; // contains another key
        }
        // Entry is free, time for CAS
        // probedKey or __array[idx] is expected to be zero
        uint32_t exp = 0;
        if (__array[idx].compare_exchange_strong(exp, elem)) {
          // successfully insert the element
          return true;
        } else {
          // some other proc got to it before us, continue searching
          // to know which elem was written to __array[idx], look at exp
          continue;
        }
      }
      return false;
    }
    return true;
  }

  bool remove(uint32_t elem) {
    for (uint32_t idx = 0; idx < size; ++idx) {
      auto probedElem = __array[idx].load();

      if (probedElem == elem) {
        return __array[idx].compare_exchange_strong(elem, 0);
        // if true, we successfully do a CAS and the element was deleted by this
        // proc if false, some other proc deleted the element instead
      }
    }

    // we exit after doing a full pass through the array
    // but we failed to delete an element, maybe already deleted
    return false;
  }

  std::atomic_uint32_t __array[size]{};
};

inline bool proc_dead(__pid_t proc) {
  if (proc == 0) {
    return false;
  }
  std::string pid_path = "/proc/" + std::to_string(proc);
  struct stat sts {};
  return (stat(pid_path.c_str(), &sts) == -1 && errno == ENOENT);
}

class IPC_Lock {
public:
  IPC_Lock() = default;
  void lock() {
    // TODO: use timed_lock instead of try_lock
    while (!mutex_.try_lock()) {
      // failed to get mutex_ within timeout,
      // so mutex_ is either held properly
      // or some process which holds it has died
      if (exclusive_owner != 0) {
        // exclusive_owner is not default value, some other proc
        // has access already
        auto ex_proc = exclusive_owner.load();
        if (proc_dead(ex_proc)) {
          // proc is fucked, we unlock
          // and continue immediately to next loop
          if (exclusive_owner.compare_exchange_strong(ex_proc, 0)) {
            mutex_.unlock();
            continue;
          }
        }
      } else {
        // exclusive_owner = 0, so the writers are blocking us
        prune_sharable_procs();
      }

      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    exclusive_owner = getpid();
  }

  void unlock() {
    __pid_t current_pid = getpid();
    if (exclusive_owner.compare_exchange_strong(current_pid, 0)) {
      mutex_.unlock();
    }
  }

  void lock_sharable() {
    // TODO: use timed_lock_sharable instead of try_lock_sharable
    while (!mutex_.try_lock_sharable()) {
      // only reason for failure is that exclusive lock is held
      if (exclusive_owner != 0) {
        auto ex_proc = exclusive_owner.load();
        if (proc_dead(ex_proc)) {
          // exclusive_owner is dead
          if (exclusive_owner.compare_exchange_strong(ex_proc, 0)) {
            exclusive_owner = 0;
            mutex_.unlock();
          }
        }
      }
      // TODO: Maybe prune_sharable_procs()?
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    while (!shared_owners.insert(getpid()))
      ;
  }

  void unlock_sharable() {
    if (shared_owners.remove(getpid())) {
      mutex_.unlock_sharable();
    }
  }

private:
  void prune_sharable_procs() {
    for (auto &i : shared_owners.__array) {
      uint32_t shared_owner = i.load();

      if (shared_owner == 0)
        continue;
      if (proc_dead(shared_owner)) {
        if (shared_owners.remove(shared_owner)) {
          // removal of element was a success
          // this ensures no over-pruning or duplicate deletion
          mutex_.unlock_sharable();
        }
      }
    }
  }

  interprocess_upgradable_mutex mutex_;
  std::atomic<__pid_t> exclusive_owner{0};
  IPC_Set<MAX_SHARED_OWNERS> shared_owners;
};
} // namespace shm
#endif // shadesmar_IPC_LOCK_H
