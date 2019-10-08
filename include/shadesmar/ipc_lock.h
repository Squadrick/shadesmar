//
// Created by squadrick on 30/09/19.
//

#ifndef SHADERMAR_IPC_LOCK_H
#define SHADERMAR_IPC_LOCK_H

#include <sys/stat.h>

#include <cstdint>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>

#include <shadesmar/macros.h>

using namespace boost::interprocess;

#define MAX_SH_PROCS 64

template <uint32_t size,
          std::memory_order mem_order = std::memory_order_relaxed>
class IPC_Set {
  static_assert((size & (size - 1)) == 0, "size must be power of two");

 public:
  IPC_Set() {
    //    for (uint32_t idx = 0; idx < size; ++idx)
    //      __array[idx] = 0;
    std::memset(__array, 0, size);
  }

  void insert(uint32_t elem) {
    DEBUG("Insert elem: " << elem);
    for (uint32_t idx = hash(elem);; ++idx) {
      idx &= size - 1;
      auto probedElem = __array[idx].load(mem_order);

      if (probedElem != elem) {
        // The entry is either free or contains another key
        if (probedElem != 0) {
          DEBUG("Contains another key");
          continue;  // contains another key
        }
        // Entry is free, time for CAS
        // probedKey or __array[idx] is expected to be zero
        uint32_t exp = 0;
        if (!__array[idx].compare_exchange_strong(exp, elem, mem_order)) {
          // some other proc got to it before us, continue searching
          // to know which elem was written to __array[idx], look at exp
          DEBUG("Other proc won the race");
          continue;
        } else {
          DEBUG("insert success");
          // successfully insert the element
          break;
        }
      }
      return;
    }
  }

  bool remove(uint32_t elem) {
    DEBUG("Remove elem: " << elem);
    uint32_t idx = hash(elem) & size - 1;
    uint32_t init_idx = idx;
    do {
      idx &= size - 1;
      auto probedElem = __array[idx].load(mem_order);

      if (probedElem == elem) {
        __array[idx].compare_exchange_strong(elem, 0, mem_order);
        DEBUG("Found and deleted");
        // successfully found and deleted elem
        return true;
      }
      idx++;

    } while (idx != init_idx);

    // we exit after doing a full pass through the array
    // but we failed to delete an element, maybe already deleted
    DEBUG("Not found");
    return false;
  }

  std::atomic_uint32_t __array[size]{};

 private:
  inline static uint32_t hash(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
  }
};

bool proc_exists(__pid_t proc) {
  std::string pid_path = "/proc/" + std::to_string(proc);
  struct stat sts {};
  return !(stat(pid_path.c_str(), &sts) == -1 && errno == ENOENT);
}

class IPC_Lock {
 public:
  IPC_Lock() = default;
  void lock() {
    auto timeout = boost::posix_time::microsec_clock::local_time() +
                   boost::posix_time::microseconds(2);
    DEBUG("Starting wait time lock");
    // TODO: use timed_lock instead of try_lock
    while (!mutex_.try_lock()) {
      // failed to get mutex_ within timeout,
      // so mutex_ is either held properly
      // or some process which holds it has died
      DEBUG("ex_proc: " << ex_proc);

      if (ex_proc != -1) {
        // ex_proc is not default value, some other proc
        // has access already
        if (proc_exists(ex_proc)) {
          DEBUG("ex_proc running: " << ex_proc);
          // fear not, everything is alright, wait for
          // next timeout
          WAIT_CONTINUE(TIMEOUT);
        } else {
          DEBUG("ex_proc dead, reset");
          // proc is fucked, we don't unlock
          // we just assume that this proc has the lock
          // relinquish control
          // and we return to the caller
          break;
        }
      } else {
        // ex_proc = -1, so the writers are blocking us
        prune_sharable_procs();
      }

      WAIT_CONTINUE(TIMEOUT);
    }
    DEBUG("lock is free");
    ex_proc = getpid();
  }

  void unlock() {
    DEBUG("Unlocking");
    ex_proc = -1;
    mutex_.unlock();
  }

  void lock_sharable() {
    auto timeout = boost::posix_time::microsec_clock::local_time() +
                   boost::posix_time::microseconds(2);
    // need to check only ex_proc
    DEBUG("Starting wait time lock");
    // TODO: use timed_lock_sharable instead of try_lock_sharable
    while (!mutex_.try_lock_sharable()) {
      DEBUG("ex_proc: " << ex_proc);
      // only reason for failure is that exclusive lock is held
      if (ex_proc == -1) {
        DEBUG("This should never happen");
        WAIT_CONTINUE(TIMEOUT);
      }

      if (proc_exists(ex_proc)) {
        // no problem, go back to waiting
        DEBUG("Go back to waiting");
        WAIT_CONTINUE(TIMEOUT);
      } else {
        DEBUG("ex_proc is dead, reset");
        ex_proc = -1;
      }

      DEBUG("maybe prune_sharable_procs");
      // TODO: Maybe prune_sharable_procs()?

      WAIT_CONTINUE(TIMEOUT);
    }
    DEBUG("sh lock is free");
    sh_procs.insert(getpid());
  }

  void unlock_sharable() {
    DEBUG("IPC: unlock share");
    sh_procs.remove(getpid());
    mutex_.unlock_sharable();
  }

  void dump() {
    DEBUG("IPC_Lock dump");
    DEBUG("ex_proc: " << ex_proc << ", status: " << proc_exists(ex_proc));

    DEBUG("sh_procs:");
    for (auto &i : sh_procs.__array) {
      uint32_t sh_proc = i.load();
      if (sh_proc == 0) continue;

      DEBUG("\t" << sh_proc << ", status: " << proc_exists(sh_proc));
    }
  }

 private:
  void prune_sharable_procs() {
    DEBUG("Pruning sharable");
    for (auto &i : sh_procs.__array) {
      uint32_t sh_proc = i.load();

      if (sh_proc == 0) continue;
      if (!proc_exists(sh_proc)) {
        if (sh_procs.remove(sh_proc)) {
          // removal of element was a success
          // this ensures no over-pruning or duplicate deletion
          // TODO: Verify correctness
          DEBUG("Sh proc pruned: " << sh_proc);
          mutex_.unlock_sharable();
        }
      }
    }
  }

  interprocess_upgradable_mutex mutex_;  // 16 bytes
  std::atomic<__pid_t> ex_proc{0};       // 4 bytes
  IPC_Set<MAX_SH_PROCS> sh_procs;        // 320 bytes
  // Total size = 340 bytes
};

#endif  // SHADERMAR_IPC_LOCK_H
