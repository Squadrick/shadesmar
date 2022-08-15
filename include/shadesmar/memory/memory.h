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

#ifndef INCLUDE_SHADESMAR_MEMORY_MEMORY_H_
#define INCLUDE_SHADESMAR_MEMORY_MEMORY_H_

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

#include "shadesmar/concurrency/lockless_set.h"
#include "shadesmar/concurrency/robust_lock.h"
#include "shadesmar/macros.h"
#include "shadesmar/memory/allocator.h"

namespace shm::memory {

static constexpr size_t QUEUE_SIZE = 1024;
static size_t buffer_size = (1U << 28);  // 256mb
static size_t GAP = 1024;                // 1kb safety gap

inline uint8_t *create_memory_segment(const std::string &name, size_t size,
                                      bool *new_segment,
                                      size_t alignment = 32) {
  /*
   * Create a new shared memory segment. The segment is created
   * under a name. We check if an existing segment is found under
   * the same name. This info is stored in `new_segment`.
   *
   * The permission of the memory segment is 0644.
   */
  int fd;
  while (true) {
    *new_segment = true;
    fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd >= 0) {
      fchmod(fd, 0644);
    } else if (errno == EEXIST) {
      fd = shm_open(name.c_str(), O_RDWR, 0644);
      if (fd < 0 && errno == ENOENT) {
        // the memory segment was deleted in the mean time
        continue;
      }
      *new_segment = false;
    } else {
      return nullptr;
    }
    break;
  }

  if (*new_segment) {
    // Allocate an extra `alignment` bytes as padding
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
    // TODO(squadrick): Make sure it is atomic
    bool alive = false;
    uint32_t current_pid = getpid();
    for (auto &i : pid_set.array_) {
      uint32_t pid = i.load();

      if (pid == 0) continue;

      if (pid == current_pid) {
        // intra-process communication
        // two threads of the same process
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
      /*
       * We do a cast first. So if the shared queue isn't init'd
       * in time, i.e., `init_shared_queue` is still running,
       * we need to wait a bit to ensure nothing crazy happens.
       * The exact time to sleep is just a guess: depends on file IO.
       * Currently: 10ms.
       */
      pid_set_ = reinterpret_cast<PIDSet *>(pid_set_address);
      shared_queue_ =
          reinterpret_cast<SharedQueue<ElemT> *>(shared_queue_address);
      allocator_ = reinterpret_cast<AllocatorT *>(allocator_address);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      /*
       * Check if any of the participating PIDs are up and running.
       * If all participating PIDs are dead, reset the underlying memory
       * structures:
       * - Reset the allocator's underlying buffer
       *   - Set the free, allocation indices to 0
       * - Reset the shared queue
       *   - Set the counter to 0
       *   - Set all underlying elements of queue to be empty
       * - Reset all mutexes/locks
       */
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
    /*
     * This is only called by the first process. It initializes the
     * counter to 0.
     */
    shared_queue_->counter = 0;
  }

  size_t queue_size() const { return QUEUE_SIZE; }

  std::string name_;
  PIDSet *pid_set_;
  AllocatorT *allocator_;
  SharedQueue<ElemT> *shared_queue_;
};

}  // namespace shm::memory

#endif  // INCLUDE_SHADESMAR_MEMORY_MEMORY_H_
