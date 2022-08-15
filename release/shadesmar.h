/*MIT License

Copyright (c) 2019 Dheeraj R Reddy

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

*/

/* 
  THIS SINGLE HEADER FILE WAS AUTO-GENERATED USING `simul/simul.py`.
  DO NOT MAKE CHANGES HERE.

  GENERATION TIME: 2022-08-15 10:40:23.622183 UTC
*/

#ifndef SHADESMAR_SINGLE_H_
#define SHADESMAR_SINGLE_H_
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
}   
#include <pthread.h>
#include <cerrno>
#include <iostream>
namespace shm::concurrent {
class PthreadWriteLock {
 public:
  PthreadWriteLock();
  ~PthreadWriteLock();
  void lock();
  bool try_lock();
  void unlock();
  void reset();
  pthread_mutex_t *get_mutex() { return &mutex; }
 private:
  pthread_mutex_t mutex{};
  pthread_mutexattr_t attr{};
};
PthreadWriteLock::PthreadWriteLock() {
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
#ifdef __linux__
  pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
#endif   
  pthread_mutex_init(&mutex, &attr);
}
PthreadWriteLock::~PthreadWriteLock() {
  pthread_mutexattr_destroy(&attr);
  pthread_mutex_destroy(&mutex);
}
void PthreadWriteLock::lock() {
  pthread_mutex_lock(&mutex);
#ifdef __linux__
  if (errno == EOWNERDEAD) {
    std::cerr << "Previous owner of mutex was dead." << std::endl;
  }
#endif   
}
bool PthreadWriteLock::try_lock() { return pthread_mutex_trylock(&mutex); }
void PthreadWriteLock::unlock() { pthread_mutex_unlock(&mutex); }
void PthreadWriteLock::reset() {
  pthread_mutex_destroy(&mutex);
  pthread_mutex_init(&mutex, &attr);
}
}   
#include <pthread.h>
namespace shm::concurrent {
class CondVar {
 public:
  CondVar();
  ~CondVar();
  void wait(PthreadWriteLock *lock);
  void signal();
  void reset();
 private:
  pthread_condattr_t attr;
  pthread_cond_t cond;
};
CondVar::CondVar() {
  pthread_condattr_init(&attr);
  pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&cond, &attr);
}
CondVar::~CondVar() {
  pthread_condattr_destroy(&attr);
  pthread_cond_destroy(&cond);
}
void CondVar::wait(PthreadWriteLock *lock) {
  pthread_cond_wait(&cond, lock->get_mutex());
}
void CondVar::signal() { pthread_cond_signal(&cond); }
void CondVar::reset() {
  pthread_cond_destroy(&cond);
  pthread_cond_init(&cond, &attr);
}
}   
#include <cstring>
#include <memory>
namespace shm::memory {
class Copier {
 public:
  virtual ~Copier() = default;
  virtual void *alloc(size_t) = 0;
  virtual void dealloc(void *) = 0;
  virtual void shm_to_user(void *, void *, size_t) = 0;
  virtual void user_to_shm(void *, void *, size_t) = 0;
};
class DefaultCopier : public Copier {
 public:
  using PtrT = uint8_t;
  void *alloc(size_t size) override { return malloc(size); }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    std::memcpy(dst, src, size);
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    std::memcpy(dst, src, size);
  }
};
}   
#include <cmath>
#include <cstdlib>
#include <iostream>
namespace shm::stats {
class Welford {
 public:
  Welford() { clear(); }
  void clear() {
    n = 0;
    old_mean = new_mean = 0.0;
    old_dev = new_dev = 0.0;
  }
  void add(double value) {
    n++;
    if (n == 1) {
      old_mean = new_mean = value;
      old_dev = 0.0;
    } else {
      new_mean = old_mean + (value - old_mean) / n;
      new_dev = old_dev + (value - old_mean) * (value - new_mean);
      old_mean = new_mean;
      old_dev = new_dev;
    }
  }
  size_t size() const { return n; }
  double mean() const {
    if (n > 0) {
      return new_mean;
    }
    return 0.0;
  }
  double variance() const {
    if (n > 1) {
      return new_dev / (n - 1);
    }
    return 0.0;
  }
  double std_dev() const { return std::sqrt(variance()); }
  friend std::ostream& operator<<(std::ostream& o, const Welford& w);
 private:
  size_t n{};
  double old_mean{}, new_mean{}, old_dev{}, new_dev{};
};
std::ostream& operator<<(std::ostream& o, const Welford& w) {
  return o << w.mean() << " ± " << w.std_dev() << " (" << w.size() << ")";
}
}   
#include <sys/stat.h>
#include <chrono>
#include <iostream>
#include <string>
#define TIMESCALE std::chrono::microseconds
#define TIMESCALE_COUNT 1e6
#define TIMESCALE_NAME "us"
namespace shm {
uint64_t current_time() {
  auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
  auto casted_time = std::chrono::duration_cast<TIMESCALE>(time_since_epoch);
  return casted_time.count();
}
}   
inline bool proc_dead(__pid_t proc) {
  if (proc == 0) {
    return false;
  }
  std::string pid_path = "/proc/" + std::to_string(proc);
  struct stat sts {};
  return (stat(pid_path.c_str(), &sts) == -1 && errno == ENOENT);
}
#include <array>
#include <atomic>
#include <cstring>
namespace shm::concurrent {
template <uint32_t Size>
class LocklessSet {
 public:
  LocklessSet();
  LocklessSet &operator=(const LocklessSet &);
  bool insert(uint32_t elem);
  bool remove(uint32_t elem);
  std::array<std::atomic_uint32_t, Size> array_ = {};
};
template <uint32_t Size>
LocklessSet<Size>::LocklessSet() = default;
template <uint32_t Size>
LocklessSet<Size> &LocklessSet<Size>::operator=(const LocklessSet &set) {
  for (uint32_t idx = 0; idx < Size; ++idx) {
    array_[idx].store(set.array_[idx].load());
  }
  return *this;
}
template <uint32_t Size>
bool LocklessSet<Size>::insert(uint32_t elem) {
  for (uint32_t idx = 0; idx < Size; ++idx) {
    auto probedElem = array_[idx].load();
    if (probedElem != elem) {
      if (probedElem != 0) {
        continue;
      }
      uint32_t exp = 0;
      if (array_[idx].compare_exchange_strong(exp, elem)) {
        return true;
      } else {
        continue;
      }
    }
    return false;
  }
  return false;
}
template <uint32_t Size>
bool LocklessSet<Size>::remove(uint32_t elem) {
  for (uint32_t idx = 0; idx < Size; ++idx) {
    auto probedElem = array_[idx].load();
    if (probedElem == elem) {
      return array_[idx].compare_exchange_strong(elem, 0);
    }
  }
  return false;
}
}   
#include <pthread.h>
namespace shm::concurrent {
class PthreadReadWriteLock {
 public:
  PthreadReadWriteLock();
  ~PthreadReadWriteLock();
  void lock();
  bool try_lock();
  void unlock();
  void lock_sharable();
  bool try_lock_sharable();
  void unlock_sharable();
  void reset();
 private:
  pthread_rwlock_t rwlock{};
  pthread_rwlockattr_t attr{};
};
PthreadReadWriteLock::PthreadReadWriteLock() {
  pthread_rwlockattr_init(&attr);
  pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_rwlock_init(&rwlock, &attr);
}
PthreadReadWriteLock::~PthreadReadWriteLock() {
  pthread_rwlockattr_destroy(&attr);
  pthread_rwlock_destroy(&rwlock);
}
void PthreadReadWriteLock::lock() { pthread_rwlock_wrlock(&rwlock); }
bool PthreadReadWriteLock::try_lock() {
  return (!pthread_rwlock_trywrlock(&rwlock));
}
void PthreadReadWriteLock::unlock() { pthread_rwlock_unlock(&rwlock); }
void PthreadReadWriteLock::lock_sharable() { pthread_rwlock_rdlock(&rwlock); }
bool PthreadReadWriteLock::try_lock_sharable() {
  return (!pthread_rwlock_tryrdlock(&rwlock));
}
void PthreadReadWriteLock::unlock_sharable() {
  pthread_rwlock_unlock(&rwlock);
}
void PthreadReadWriteLock::reset() {
  pthread_rwlock_destroy(&rwlock);
  pthread_rwlock_init(&rwlock, &attr);
}
}   
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <string>
#include <thread>
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
}   
#include <cassert>
namespace shm::memory {
#define SHMALIGN(s, a) (((s - 1) | (a - 1)) + 1)
inline uint8_t *align_address(void *ptr, size_t alignment) {
  auto int_ptr = reinterpret_cast<uintptr_t>(ptr);
  auto aligned_int_ptr = SHMALIGN(int_ptr, alignment);
  return reinterpret_cast<uint8_t *>(aligned_int_ptr);
}
class Allocator {
 public:
  using handle = uint64_t;
  template <concurrent::ExlOrShr type>
  using Scope = concurrent::ScopeGuard<concurrent::RobustLock, type>;
  Allocator(size_t offset, size_t size);
  uint8_t *alloc(uint32_t bytes);
  uint8_t *alloc(uint32_t bytes, size_t alignment);
  bool free(const uint8_t *ptr);
  void reset();
  void lock_reset();
  inline handle ptr_to_handle(uint8_t *p) {
    return p - reinterpret_cast<uint8_t *>(heap_());
  }
  uint8_t *handle_to_ptr(handle h) {
    return reinterpret_cast<uint8_t *>(heap_()) + h;
  }
  size_t get_free_memory() {
    Scope<concurrent::SHARED> _(&lock_);
    size_t free_size;
    size_t size = size_ / sizeof(int);
    if (free_index_ <= alloc_index_) {
      free_size = size - alloc_index_ + free_index_;
    } else {
      free_size = free_index_ - alloc_index_;
    }
    return free_size * sizeof(int);
  }
  concurrent::RobustLock lock_;
 private:
  void validate_index(uint32_t index) const;
  [[nodiscard]] uint32_t suggest_index(uint32_t header_index,
                                       uint32_t payload_size) const;
  uint32_t *__attribute__((always_inline)) heap_() {
    return reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(this) +
                                        offset_);
  }
  uint32_t alloc_index_;
  volatile uint32_t free_index_;
  size_t offset_;
  size_t size_;
};
Allocator::Allocator(size_t offset, size_t size)
    : alloc_index_(0), free_index_(0), offset_(offset), size_(size) {
  assert(!(size & (sizeof(int) - 1)));
}
void Allocator::validate_index(uint32_t index) const {
  assert(index < (size_ / sizeof(int)));
}
uint32_t Allocator::suggest_index(uint32_t header_index,
                                  uint32_t payload_size) const {
  validate_index(header_index);
  int32_t payload_index = header_index + 1;
  if (payload_index + payload_size - 1 >= size_ / sizeof(int)) {
    payload_index = 0;
  }
  validate_index(payload_index);
  validate_index(payload_index + payload_size - 1);
  return payload_index;
}
uint8_t *Allocator::alloc(uint32_t bytes) { return alloc(bytes, 1); }
uint8_t *Allocator::alloc(uint32_t bytes, size_t alignment) {
  uint32_t payload_size = bytes + alignment;
  if (payload_size == 0) {
    payload_size = sizeof(int);
  }
  if (payload_size >= size_ - 2 * sizeof(int)) {
    return nullptr;
  }
  if (payload_size & (sizeof(int) - 1)) {
    payload_size &= ~(sizeof(int) - 1);
    payload_size += sizeof(int);
  }
  payload_size /= sizeof(int);
  Scope<concurrent::EXCLUSIVE> _(&lock_);
  const auto payload_index = suggest_index(alloc_index_, payload_size);
  const auto free_index_th = free_index_;
  uint32_t new_alloc_index = payload_index + payload_size;
  if (alloc_index_ < free_index_th && payload_index == 0) {
    return nullptr;
  }
  if (payload_index <= free_index_th && free_index_th <= new_alloc_index) {
    return nullptr;
  }
  if (new_alloc_index == size_ / sizeof(int)) {
    new_alloc_index = 0;
    if (new_alloc_index == free_index_th) {
      return nullptr;
    }
  }
  assert(new_alloc_index != alloc_index_);
  validate_index(new_alloc_index);
  heap_()[alloc_index_] = payload_size;
  alloc_index_ = new_alloc_index;
  auto heap_ptr = reinterpret_cast<void *>(heap_() + payload_index);
  return align_address(heap_ptr, alignment);
}
bool Allocator::free(const uint8_t *ptr) {
  if (ptr == nullptr) {
    return true;
  }
  auto *heap = reinterpret_cast<uint8_t *>(heap_());
  Scope<concurrent::EXCLUSIVE> _(&lock_);
  assert(ptr >= heap);
  assert(ptr < heap + size_);
  validate_index(free_index_);
  uint32_t payload_size = heap_()[free_index_];
  uint32_t payload_index = suggest_index(free_index_, payload_size);
  if (ptr != reinterpret_cast<uint8_t *>(heap_() + payload_index)) {
    return false;
  }
  uint32_t new_free_index = payload_index + payload_size;
  if (new_free_index == size_ / sizeof(int)) {
    new_free_index = 0;
  }
  free_index_ = new_free_index;
  return true;
}
void Allocator::reset() {
  alloc_index_ = 0;
  free_index_ = 0;
}
void Allocator::lock_reset() { lock_.reset(); }
}   
namespace shm::memory {
class DoubleAllocator {
 public:
  DoubleAllocator(size_t offset, size_t size)
      : req(offset, size / 2), resp(offset + size / 2, size / 2) {}
  Allocator req;
  Allocator resp;
  void reset() {
    req.reset();
    resp.reset();
  }
  void lock_reset() {
    req.lock_reset();
    resp.lock_reset();
  }
};
}   
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
namespace shm::memory {
static constexpr size_t QUEUE_SIZE = 1024;
static size_t buffer_size = (1U << 28);   
static size_t GAP = 1024;                 
inline uint8_t *create_memory_segment(const std::string &name, size_t size,
                                      bool *new_segment,
                                      size_t alignment = 32) {
  int fd;
  while (true) {
    *new_segment = true;
    fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd >= 0) {
      fchmod(fd, 0644);
    } else if (errno == EEXIST) {
      fd = shm_open(name.c_str(), O_RDWR, 0644);
      if (fd < 0 && errno == ENOENT) {
        continue;
      }
      *new_segment = false;
    } else {
      return nullptr;
    }
    break;
  }
  if (*new_segment) {
    int result = ftruncate(fd, size + alignment);
    if (result == EINVAL) {
      return nullptr;
    }
  }
  auto *ptr = mmap(nullptr, size + alignment, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);
  return align_address(ptr, alignment);
}
struct Memblock {
  void *ptr;
  size_t size;
  bool free;
  Memblock() : ptr(nullptr), size(0), free(true) {}
  Memblock(void *ptr, size_t size) : ptr(ptr), size(size), free(true) {}
  void no_delete() { free = false; }
  bool is_empty() const { return ptr == nullptr && size == 0; }
};
class PIDSet {
 public:
  bool any_alive() {
    bool alive = false;
    uint32_t current_pid = getpid();
    for (auto &i : pid_set.array_) {
      uint32_t pid = i.load();
      if (pid == 0) continue;
      if (pid == current_pid) {
        alive = true;
      }
      if (proc_dead(pid)) {
        while (!pid_set.remove(pid)) {
        }
      } else {
        alive = true;
      }
    }
    return alive;
  }
  bool insert(uint32_t pid) { return pid_set.insert(pid); }
  void lock() { lck.lock(); }
  void unlock() { lck.unlock(); }
 private:
  concurrent::LocklessSet<32> pid_set;
  concurrent::RobustLock lck;
};
struct Element {
  size_t size;
  bool empty;
  Allocator::handle address_handle;
  Element() : size(0), address_handle(0), empty(true) {}
  void reset() {
    size = 0;
    address_handle = 0;
    empty = true;
  }
};
template <class ElemT>
class SharedQueue {
 public:
  std::atomic<uint32_t> counter;
  std::array<ElemT, QUEUE_SIZE> elements;
};
template <class ElemT, class AllocatorT>
class Memory {
 public:
  explicit Memory(const std::string &name) : name_(name) {
    auto pid_set_size = sizeof(PIDSet);
    auto shared_queue_size = sizeof(SharedQueue<ElemT>);
    auto allocator_size = sizeof(AllocatorT);
    auto total_size = pid_set_size + shared_queue_size + allocator_size +
                      buffer_size + 4 * GAP;
    bool new_segment = false;
    auto *base_address =
        create_memory_segment("/SHM_" + name, total_size, &new_segment);
    if (base_address == nullptr) {
      std::cerr << "Could not create/open shared memory segment.\n";
      exit(1);
    }
    auto *pid_set_address = base_address;
    auto *shared_queue_address = pid_set_address + pid_set_size + GAP;
    auto *allocator_address = shared_queue_address + shared_queue_size + GAP;
    auto *buffer_address = allocator_address + allocator_size + GAP;
    if (new_segment) {
      pid_set_ = new (pid_set_address) PIDSet();
      shared_queue_ = new (shared_queue_address) SharedQueue<ElemT>();
      allocator_ = new (allocator_address)
          AllocatorT(buffer_address - allocator_address, buffer_size);
      pid_set_->insert(getpid());
      init_shared_queue();
    } else {
      pid_set_ = reinterpret_cast<PIDSet *>(pid_set_address);
      shared_queue_ =
          reinterpret_cast<SharedQueue<ElemT> *>(shared_queue_address);
      allocator_ = reinterpret_cast<AllocatorT *>(allocator_address);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      pid_set_->lock();
      if (!pid_set_->any_alive()) {
        allocator_->lock_reset();
        for (auto &elem : shared_queue_->elements) {
          elem.reset();
        }
        shared_queue_->counter = 0;
        allocator_->reset();
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      pid_set_->unlock();
      pid_set_->insert(getpid());
    }
  }
  ~Memory() = default;
  void init_shared_queue() {
    shared_queue_->counter = 0;
  }
  size_t queue_size() const { return QUEUE_SIZE; }
  std::string name_;
  PIDSet *pid_set_;
  AllocatorT *allocator_;
  SharedQueue<ElemT> *shared_queue_;
};
}   
#include <atomic>
#include <memory>
#include <string>
namespace shm::rpc {
struct ChannelElem {
  memory::Element req;
  memory::Element resp;
  concurrent::PthreadWriteLock mutex;
  concurrent::CondVar cond_var;
  ChannelElem() : req(), resp(), mutex(), cond_var() {}
  ChannelElem(const ChannelElem &channel_elem) {
    mutex = channel_elem.mutex;
    cond_var = channel_elem.cond_var;
    req = channel_elem.req;
    resp = channel_elem.resp;
  }
  void reset() {
    req.reset();
    resp.reset();
    mutex.reset();
    cond_var.reset();
  }
};
class Channel {
  using Scope = concurrent::ScopeGuard<concurrent::PthreadWriteLock,
                                       concurrent::EXCLUSIVE>;
 public:
  explicit Channel(const std::string &channel)
      : Channel(channel, std::make_shared<memory::DefaultCopier>()) {}
  Channel(const std::string &channel, std::shared_ptr<memory::Copier> copier)
      : memory_(channel) {
    if (copier == nullptr) {
      copier = std::make_shared<memory::DefaultCopier>();
    }
    copier_ = copier;
  }
  ~Channel() = default;
  bool write_client(memory::Memblock memblock, uint32_t *pos) {
    if (memblock.size > memory_.allocator_->req.get_free_memory()) {
      std::cerr << "Increase buffer_size" << std::endl;
      return false;
    }
    *pos = counter();
    auto q_pos = *pos & (queue_size() - 1);
    ChannelElem *elem = &(memory_.shared_queue_->elements[q_pos]);
    {
      Scope _(&elem->mutex);
      if (!elem->req.empty) {
        std::cerr << "Queue is full, try again." << std::endl;
        return false;
      }
      inc_counter();   
      elem->req.empty = false;
    }
    uint8_t *new_address = memory_.allocator_->req.alloc(memblock.size);
    if (new_address == nullptr) {
      elem->req.reset();
      return false;
    }
    copier_->user_to_shm(new_address, memblock.ptr, memblock.size);
    {
      elem->req.address_handle =
          memory_.allocator_->req.ptr_to_handle(new_address);
      elem->req.size = memblock.size;
    }
    return true;
  }
  bool read_client(uint32_t pos, memory::Memblock *memblock) {
    uint32_t q_pos = pos & (queue_size() - 1);
    ChannelElem *elem = &(memory_.shared_queue_->elements[q_pos]);
    Scope _(&elem->mutex);
    while (elem->resp.empty) {
      elem->cond_var.wait(&elem->mutex);
    }
    auto clean_up = [this](ChannelElem *elem) {
      if (elem->req.size != 0) {
        auto address =
            memory_.allocator_->req.handle_to_ptr(elem->req.address_handle);
        memory_.allocator_->req.free(address);
      }
      elem->resp.reset();
      elem->req.reset();
    };
    if (elem->resp.address_handle == 0) {
      clean_up(elem);
      return false;
    }
    uint8_t *address =
        memory_.allocator_->resp.handle_to_ptr(elem->resp.address_handle);
    memblock->size = elem->resp.size;
    memblock->ptr = copier_->alloc(memblock->size);
    copier_->shm_to_user(memblock->ptr, address, memblock->size);
    clean_up(elem);
    return true;
  }
  bool write_server(memory::Memblock memblock, uint32_t pos) {
    uint32_t q_pos = pos & (queue_size() - 1);
    ChannelElem *elem = &(memory_.shared_queue_->elements[q_pos]);
    auto signal_error = [](ChannelElem *elem) {
      Scope _(&elem->mutex);
      elem->resp.reset();
      elem->resp.empty = false;
      elem->cond_var.signal();
    };
    if (memblock.size > memory_.allocator_->resp.get_free_memory()) {
      std::cerr << "Increase buffer_size" << std::endl;
      signal_error(elem);
      return false;
    }
    uint8_t *resp_address = memory_.allocator_->resp.alloc(memblock.size);
    if (resp_address == nullptr) {
      std::cerr << "Failed to alloc resp buffer" << std::endl;
      signal_error(elem);
      return false;
    }
    copier_->user_to_shm(resp_address, memblock.ptr, memblock.size);
    Scope _(&elem->mutex);
    elem->resp.empty = false;
    elem->resp.address_handle =
        memory_.allocator_->resp.ptr_to_handle(resp_address);
    elem->resp.size = memblock.size;
    elem->cond_var.signal();
    return true;
  }
  bool read_server(uint32_t pos, memory::Memblock *memblock) {
    uint32_t q_pos = pos & (queue_size() - 1);
    ChannelElem *elem = &(memory_.shared_queue_->elements[q_pos]);
    Scope _(&elem->mutex);
    if (elem->req.empty) {
      return false;
    }
    uint8_t *address =
        memory_.allocator_->req.handle_to_ptr(elem->req.address_handle);
    memblock->size = elem->req.size;
    memblock->ptr = copier_->alloc(memblock->size);
    copier_->shm_to_user(memblock->ptr, address, memblock->size);
    return true;
  }
  inline __attribute__((always_inline)) void inc_counter() {
    memory_.shared_queue_->counter++;
  }
  inline __attribute__((always_inline)) uint32_t counter() const {
    return memory_.shared_queue_->counter.load();
  }
  size_t queue_size() const { return memory_.queue_size(); }
  inline std::shared_ptr<memory::Copier> copier() const { return copier_; }
 private:
  memory::Memory<ChannelElem, memory::DoubleAllocator> memory_;
  std::shared_ptr<memory::Copier> copier_;
};
}   
#include <memory>
#include <string>
#include <utility>
namespace shm::rpc {
class Client {
 public:
  explicit Client(const std::string &channel_name);
  Client(const std::string &channel_name,
         std::shared_ptr<memory::Copier> copier);
  Client(const Client &) = delete;
  Client(Client &&);
  bool call(const memory::Memblock &req, memory::Memblock *resp) const;
  bool send(const memory::Memblock &req, uint32_t *pos) const;
  bool recv(uint32_t pos, memory::Memblock *resp) const;
  void free_resp(memory::Memblock *resp) const;
 private:
  std::string channel_name_;
  std::unique_ptr<Channel> channel_;
};
Client::Client(const std::string &channel_name) : channel_name_(channel_name) {
  channel_ = std::make_unique<Channel>(channel_name);
}
Client::Client(const std::string &channel_name,
               std::shared_ptr<memory::Copier> copier)
    : channel_name_(channel_name) {
  channel_ = std::make_unique<Channel>(channel_name, copier);
}
Client::Client(Client &&other) {
  channel_name_ = other.channel_name_;
  channel_ = std::move(other.channel_);
}
bool Client::call(const memory::Memblock &req, memory::Memblock *resp) const {
  uint32_t pos;
  bool success = send(req, &pos);
  if (!success) return success;
  return recv(pos, resp);
}
bool Client::send(const memory::Memblock &req, uint32_t *pos) const {
  return channel_->write_client(req, pos);
}
bool Client::recv(uint32_t pos, memory::Memblock *resp) const {
  return channel_->read_client(pos, resp);
}
void Client::free_resp(memory::Memblock *resp) const {
  channel_->copier()->dealloc(resp->ptr);
  resp->ptr = nullptr;
  resp->size = 0;
}
}   
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>
namespace shm::rpc {
using Callback =
    std::function<bool(const memory::Memblock& req, memory::Memblock* resp)>;
using Cleanup = std::function<void(memory::Memblock* resp)>;
inline Cleanup empty_cleanup() {
  return [](memory::Memblock* resp) {
    resp->ptr = nullptr;
    resp->size = 0;
  };
}
class Server {
 public:
  Server(const std::string& channel_name, Callback cb);
  Server(const std::string& channel_name, Callback cb, Cleanup cln);
  Server(const std::string& channel_name, Callback cb, Cleanup cln,
         std::shared_ptr<memory::Copier> copier);
  Server(const Server& other) = delete;
  Server(Server&& other);
  bool serve_once();
  void serve();
  void stop();
 private:
  bool process(uint32_t pos) const;
  void cleanup_req(memory::Memblock*) const;
  std::atomic_uint32_t pos_{0};
  std::atomic_bool running_{false};
  Callback callback_;
  Cleanup cleanup_;
  std::string channel_name_;
  std::unique_ptr<Channel> channel_;
};
Server::Server(const std::string& channel_name, Callback cb)
    : channel_name_(channel_name),
      callback_(std::move(cb)),
      cleanup_(std::move(empty_cleanup())) {
  channel_ = std::make_unique<Channel>(channel_name);
}
Server::Server(const std::string& channel_name, Callback cb, Cleanup cleanup)
    : channel_name_(channel_name),
      callback_(std::move(cb)),
      cleanup_(cleanup) {
  channel_ = std::make_unique<Channel>(channel_name);
}
Server::Server(const std::string& channel_name, Callback cb, Cleanup cleanup,
               std::shared_ptr<memory::Copier> copier)
    : channel_name_(channel_name),
      callback_(std::move(cb)),
      cleanup_(cleanup) {
  channel_ = std::make_unique<Channel>(channel_name, copier);
}
Server::Server(Server&& other) {
  callback_ = std::move(other.callback_);
  cleanup_ = std::move(other.cleanup_);
  channel_ = std::move(other.channel_);
}
void Server::cleanup_req(memory::Memblock* req) const {
  if (req->is_empty()) return;
  channel_->copier()->dealloc(req->ptr);
  *req = memory::Memblock(nullptr, 0);
}
bool Server::process(uint32_t pos) const {
  memory::Memblock req, resp;
  bool success = channel_->read_server(pos, &req);
  if (!success) {
    cleanup_req(&req);
    return success;
  }
  bool cbSuccess = callback_(req, &resp);
  if (!cbSuccess) {
    cleanup_(&resp);
    resp = memory::Memblock(nullptr, 0);
    cleanup_req(&req);
  }
  success = channel_->write_server(resp, pos);
  cleanup_req(&req);
  cleanup_(&resp);
  return success;
}
bool Server::serve_once() {
  bool success = process(pos_.fetch_add(1));
  return success;
}
void Server::serve() {
  running_ = true;
  while (running_.load()) {
    serve_once();
  }
}
void Server::stop() { running_ = false; }
}   
#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
namespace shm::pubsub {
inline uint32_t jumpahead(uint32_t counter, uint32_t queue_size) {
  return counter - queue_size / 2;
}
template <class LockT>
struct TopicElemT {
  memory::Element msg;
  LockT mutex;
  TopicElemT() : msg(), mutex() {}
  TopicElemT(const TopicElemT &topic_elem) {
    msg = topic_elem.msg;
    mutex = topic_elem.mutex;
  }
  void reset() {
    msg.reset();
    mutex.reset();
  }
};
using LockType = concurrent::PthreadReadWriteLock;
class Topic {
  using TopicElem = TopicElemT<LockType>;
  template <concurrent::ExlOrShr type>
  using Scope = concurrent::ScopeGuard<LockType, type>;
 public:
  explicit Topic(const std::string &topic)
      : Topic(topic, std::make_shared<memory::DefaultCopier>()) {}
  Topic(const std::string &topic, std::shared_ptr<memory::Copier> copier)
      : memory_(topic) {
    if (copier == nullptr) {
      copier = std::make_shared<memory::DefaultCopier>();
    }
    copier_ = copier;
  }
  ~Topic() = default;
  bool write(memory::Memblock memblock) {
    if (memblock.size > memory_.allocator_->get_free_memory()) {
      std::cerr << "Increase buffer_size" << std::endl;
      return false;
    }
    uint32_t q_pos = counter() & (queue_size() - 1);
    TopicElem *elem = &(memory_.shared_queue_->elements[q_pos]);
    uint8_t *new_address = memory_.allocator_->alloc(memblock.size);
    if (new_address == nullptr) {
      return false;
    }
    copier_->user_to_shm(new_address, memblock.ptr, memblock.size);
    uint8_t *old_address = nullptr;
    {
      Scope<concurrent::EXCLUSIVE> _(&elem->mutex);
      if (!elem->msg.empty) {
        old_address =
            memory_.allocator_->handle_to_ptr(elem->msg.address_handle);
      }
      elem->msg.address_handle =
          memory_.allocator_->ptr_to_handle(new_address);
      elem->msg.size = memblock.size;
      elem->msg.empty = false;
    }
    while (!memory_.allocator_->free(old_address)) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    inc_counter();
    return true;
  }
  bool read(memory::Memblock *memblock, std::atomic<uint32_t> *pos) {
    TopicElem *elem =
        &(memory_.shared_queue_->elements[*pos & (queue_size() - 1)]);
    Scope<concurrent::SHARED> _(&elem->mutex);
#define MOVE_ELEM(_elem)                                                    \
  if (_elem->msg.empty) {                                                   \
    return false;                                                           \
  }                                                                         \
  auto *dst = memory_.allocator_->handle_to_ptr(_elem->msg.address_handle); \
  memblock->size = _elem->msg.size;                                         \
  memblock->ptr = copier_->alloc(memblock->size);                           \
  copier_->shm_to_user(memblock->ptr, dst, memblock->size);
    if (queue_size() > counter() - *pos) {
      MOVE_ELEM(elem);
      return true;
    }
    *pos = jumpahead(counter(), queue_size());
    TopicElem *next_best_elem =
        &(memory_.shared_queue_->elements[*pos & (queue_size() - 1)]);
    MOVE_ELEM(next_best_elem);
    return true;
#undef MOVE_ELEM
  }
  inline __attribute__((always_inline)) void inc_counter() {
    memory_.shared_queue_->counter++;
  }
  inline __attribute__((always_inline)) uint32_t counter() const {
    return memory_.shared_queue_->counter.load();
  }
  size_t queue_size() const { return memory_.queue_size(); }
  inline std::shared_ptr<memory::Copier> copier() const { return copier_; }
 private:
  memory::Memory<TopicElem, memory::Allocator> memory_;
  std::shared_ptr<memory::Copier> copier_;
};
}   
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
namespace shm::pubsub {
class Publisher {
 public:
  explicit Publisher(const std::string &topic_name);
  Publisher(const std::string &topic_name,
            std::shared_ptr<memory::Copier> copier);
  Publisher(const Publisher &) = delete;
  Publisher(Publisher &&);
  bool publish(void *data, size_t size);
 private:
  std::string topic_name_;
  std::unique_ptr<Topic> topic_;
};
Publisher::Publisher(const std::string &topic_name) : topic_name_(topic_name) {
  topic_ = std::make_unique<Topic>(topic_name);
}
Publisher::Publisher(const std::string &topic_name,
                     std::shared_ptr<memory::Copier> copier)
    : topic_name_(topic_name) {
  topic_ = std::make_unique<Topic>(topic_name, copier);
}
Publisher::Publisher(Publisher &&other) {
  topic_name_ = other.topic_name_;
  topic_ = std::move(other.topic_);
}
bool Publisher::publish(void *data, size_t size) {
  memory::Memblock memblock(data, size);
  return topic_->write(memblock);
}
}   
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
namespace shm::pubsub {
class Subscriber {
 public:
  Subscriber(const std::string &topic_name,
             std::function<void(memory::Memblock *)> callback);
  Subscriber(const std::string &topic_name,
             std::function<void(memory::Memblock *)> callback,
             std::shared_ptr<memory::Copier> copier);
  Subscriber(const Subscriber &other) = delete;
  Subscriber(Subscriber &&other);
  memory::Memblock get_message();
  void spin_once();
  void spin();
  void stop();
  std::atomic<uint32_t> counter_{0};
 private:
  std::atomic_bool running_{false};
  std::function<void(memory::Memblock *)> callback_;
  std::string topic_name_;
  std::unique_ptr<Topic> topic_;
};
Subscriber::Subscriber(const std::string &topic_name,
                       std::function<void(memory::Memblock *)> callback)
    : topic_name_(topic_name), callback_(std::move(callback)) {
  topic_ = std::make_unique<Topic>(topic_name_);
}
Subscriber::Subscriber(const std::string &topic_name,
                       std::function<void(memory::Memblock *)> callback,
                       std::shared_ptr<memory::Copier> copier)
    : topic_name_(topic_name), callback_(std::move(callback)) {
  topic_ = std::make_unique<Topic>(topic_name_, copier);
}
Subscriber::Subscriber(Subscriber &&other) {
  callback_ = std::move(other.callback_);
  topic_ = std::move(other.topic_);
}
memory::Memblock Subscriber::get_message() {
  if (topic_->counter() <= counter_) {
    return memory::Memblock();
  }
  if (topic_->counter() - counter_ >= topic_->queue_size()) {
    counter_ = jumpahead(topic_->counter(), topic_->queue_size());
  }
  memory::Memblock memblock;
  memblock.free = true;
  if (!topic_->read(&memblock, &counter_)) {
    return memory::Memblock();
  }
  return memblock;
}
void Subscriber::spin_once() {
  memory::Memblock memblock = get_message();
  if (memblock.is_empty()) {
    return;
  }
  callback_(&memblock);
  counter_++;
  if (memblock.free) {
    topic_->copier()->dealloc(memblock.ptr);
    memblock.ptr = nullptr;
    memblock.size = 0;
  }
}
void Subscriber::spin() {
  running_ = true;
  while (running_.load()) {
    spin_once();
  }
}
void Subscriber::stop() { running_ = false; }
}   
#include <immintrin.h>
#include <cstdlib>
#include <thread>
#include <vector>
namespace shm::memory::dragons {
#ifdef __x86_64__
static inline void _rep_movsb(void *d, const void *s, size_t n) {
  asm volatile("rep movsb"
               : "=D"(d), "=S"(s), "=c"(n)
               : "0"(d), "1"(s), "2"(n)
               : "memory");
}
class RepMovsbCopier : public Copier {
 public:
  using PtrT = uint8_t;
  void *alloc(size_t size) override { return malloc(size); }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _rep_movsb(dst, src, size);
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _rep_movsb(dst, src, size);
  }
};
#endif   
#ifdef __AVX__
static inline void _avx_cpy(void *d, const void *s, size_t n) {
  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec--, sVec++, dVec++) {
    const __m256i temp = _mm256_load_si256(sVec);
    _mm256_store_si256(dVec, temp);
  }
}
class AvxCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = sizeof(__m256i);
  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_cpy(dst, src, SHMALIGN(size, alignment));
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_cpy(dst, src, SHMALIGN(size, alignment));
  }
};
#endif   
#ifdef __AVX2__
static inline void _avx_async_cpy(void *d, const void *s, size_t n) {
  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec--, sVec++, dVec++) {
    const __m256i temp = _mm256_stream_load_si256(sVec);
    _mm256_stream_si256(dVec, temp);
  }
  _mm_sfence();
}
class AvxAsyncCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = sizeof(__m256i);
  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_cpy(dst, src, SHMALIGN(size, alignment));
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_cpy(dst, src, SHMALIGN(size, alignment));
  }
};
#endif   
#ifdef __AVX2__
static inline void _avx_async_pf_cpy(void *d, const void *s, size_t n) {
  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 2; nVec -= 2, sVec += 2, dVec += 2) {
    _mm_prefetch(sVec + 2, _MM_HINT_T0);
    _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
    _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
  }
  _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
  _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
  _mm_sfence();
}
class AvxAsyncPFCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = sizeof(__m256i) * 2;
  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy(dst, src, SHMALIGN(size, alignment));
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy(dst, src, SHMALIGN(size, alignment));
  }
};
#endif   
#ifdef __AVX__
static inline void _avx_cpy_unroll(void *d, const void *s, size_t n) {
  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec -= 4, sVec += 4, dVec += 4) {
    _mm256_store_si256(dVec, _mm256_load_si256(sVec));
    _mm256_store_si256(dVec + 1, _mm256_load_si256(sVec + 1));
    _mm256_store_si256(dVec + 2, _mm256_load_si256(sVec + 2));
    _mm256_store_si256(dVec + 3, _mm256_load_si256(sVec + 3));
  }
}
class AvxUnrollCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = 4 * sizeof(__m256i);
  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
};
#endif   
#ifdef __AVX2__
static inline void _avx_async_cpy_unroll(void *d, const void *s, size_t n) {
  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec -= 4, sVec += 4, dVec += 4) {
    _mm256_stream_si256(dVec, _mm256_stream_load_si256(sVec));
    _mm256_stream_si256(dVec + 1, _mm256_stream_load_si256(sVec + 1));
    _mm256_stream_si256(dVec + 2, _mm256_stream_load_si256(sVec + 2));
    _mm256_stream_si256(dVec + 3, _mm256_stream_load_si256(sVec + 3));
  }
  _mm_sfence();
}
class AvxAsyncUnrollCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = 4 * sizeof(__m256i);
  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
};
#endif   
#ifdef __AVX2__
static inline void _avx_async_pf_cpy_unroll(void *d, const void *s, size_t n) {
  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 4; nVec -= 4, sVec += 4, dVec += 4) {
    _mm_prefetch(sVec + 4, _MM_HINT_T0);
    _mm_prefetch(sVec + 6, _MM_HINT_T0);
    _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
    _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
    _mm256_stream_si256(dVec + 2, _mm256_load_si256(sVec + 2));
    _mm256_stream_si256(dVec + 3, _mm256_load_si256(sVec + 3));
  }
  _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
  _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
  _mm256_stream_si256(dVec + 2, _mm256_load_si256(sVec + 2));
  _mm256_stream_si256(dVec + 3, _mm256_load_si256(sVec + 3));
  _mm_sfence();
}
class AvxAsyncPFUnrollCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = 4 * sizeof(__m256i);
  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }
  void dealloc(void *ptr) override { free(ptr); }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
};
#endif   
template <class BaseCopierT, uint32_t nthreads>
class MTCopier : public Copier {
 public:
  MTCopier() : base_copier() {}
  void *alloc(size_t size) override { return base_copier.alloc(size); }
  void dealloc(void *ptr) override { base_copier.dealloc(ptr); }
  void _copy(void *d, void *s, size_t n, bool shm_to_user) {
    n = SHMALIGN(n, sizeof(typename BaseCopierT::PtrT)) /
        sizeof(typename BaseCopierT::PtrT);
    std::vector<std::thread> threads;
    threads.reserve(nthreads);
    auto per_worker = div((int64_t)n, nthreads);
    size_t next_start = 0;
    for (uint32_t thread_idx = 0; thread_idx < nthreads; ++thread_idx) {
      const size_t curr_start = next_start;
      next_start += per_worker.quot;
      if (thread_idx < per_worker.rem) {
        ++next_start;
      }
      auto d_thread =
          reinterpret_cast<typename BaseCopierT::PtrT *>(d) + curr_start;
      auto s_thread =
          reinterpret_cast<typename BaseCopierT::PtrT *>(s) + curr_start;
      if (shm_to_user) {
        threads.emplace_back(
            &Copier::shm_to_user, &base_copier, d_thread, s_thread,
            (next_start - curr_start) * sizeof(typename BaseCopierT::PtrT));
      } else {
        threads.emplace_back(
            &Copier::user_to_shm, &base_copier, d_thread, s_thread,
            (next_start - curr_start) * sizeof(typename BaseCopierT::PtrT));
      }
    }
    for (auto &thread : threads) {
      thread.join();
    }
    threads.clear();
  }
  void shm_to_user(void *dst, void *src, size_t size) override {
    _copy(dst, src, size, true);
  }
  void user_to_shm(void *dst, void *src, size_t size) override {
    _copy(dst, src, size, false);
  }
 private:
  BaseCopierT base_copier;
};
}   
#endif