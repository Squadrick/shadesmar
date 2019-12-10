//
// Created by squadrick on 11/12/19.
//

#ifndef SHADESMAR_LOCKLESS_SET_H
#define SHADESMAR_LOCKLESS_SET_H

#include <atomic>
#include <cstring>

#include <shadesmar/macros.h>

#ifdef __APPLE__
#define __pid_t __darwin_pid_t
#endif

class LocklessSet {
public:
  LocklessSet();

  bool insert(uint32_t elem);
  bool remove(uint32_t elem);

  std::atomic_uint32_t __array[MAX_SHARED_OWNERS]{};
};

LocklessSet::LocklessSet() { std::memset(__array, 0, MAX_SHARED_OWNERS); }

bool LocklessSet::insert(uint32_t elem) {
  for (uint32_t idx = 0; idx < MAX_SHARED_OWNERS; ++idx) {
    auto probedElem = __array[idx].load();

    if (probedElem != elem) {
      if (probedElem != 0) {
        continue;
      }
      uint32_t exp = 0;
      if (__array[idx].compare_exchange_strong(exp, elem)) {
        return true;
      } else {
        continue;
      }
    }
    return false;
  }
  return true;
}

bool LocklessSet::remove(uint32_t elem) {
  for (uint32_t idx = 0; idx < MAX_SHARED_OWNERS; ++idx) {
    auto probedElem = __array[idx].load();

    if (probedElem == elem) {
      return __array[idx].compare_exchange_strong(elem, 0);
    }
  }

  return false;
}

#endif // SHADESMAR_LOCKLESS_SET_H
