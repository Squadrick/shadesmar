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

#include "shadesmar/concurrency/robust_lock.h"
#include "shadesmar/macros.h"
#include "shadesmar/memory/allocator.h"
#include "shadesmar/memory/tmp.h"

namespace shm::memory {

#define SHMALIGN(s, a) (((s - 1) | (a - 1)) + 1)

uint8_t *create_memory_segment(const std::string &name, size_t size,
                               bool *new_segment, size_t alignment = 32) {
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
    }
    if (errno == EEXIST) {
      fd = shm_open(name.c_str(), O_RDWR, 0644);
      if (fd < 0 && errno == ENOENT) {
        // the memory segment was deleted in the mean time
        continue;
      }
      *new_segment = false;
    }
    break;
  }

  // We allocate an extra `alignment` bytes as padding
  int result = ftruncate(fd, size + alignment);
  if (result == EINVAL) {
    return nullptr;
  }

  auto *ptr = mmap(nullptr, size + alignment, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);
  auto intptr = reinterpret_cast<uintptr_t>(ptr);
  uintptr_t aligned_ptr = SHMALIGN(intptr, alignment);
  return reinterpret_cast<uint8_t *>(aligned_ptr);
}

struct Memblock {
  void *ptr;
  size_t size;
  bool free;

  Memblock() : ptr(nullptr), size(0), free(true) {}
  Memblock(void *ptr, size_t size) : ptr(ptr), size(size), free(true) {}

  void no_delete() { free = false; }
};

struct Element {
  size_t size;
  bool empty;
  Allocator::handle address_handle;

  Element() : size(0), address_handle(0), empty(true) {}
};

template <class ElemT, uint32_t queue_size>
class SharedQueue {
 public:
  std::atomic<uint32_t> counter;  // 8 bytes
  std::array<ElemT, queue_size> elements;
};

static size_t buffer_size = (1U << 28);  // 256mb
static size_t GAP = 1024;                // safety gaps

template <class ElemT, uint32_t queue_size = 1>
class Memory {
  static_assert(std::is_base_of<Element, ElemT>::value,
                "ElemT must be a subclass of Element");

  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

 public:
  explicit Memory(const std::string &name) : name_(name) {
    auto shared_queue_size = sizeof(SharedQueue<ElemT, queue_size>);
    auto allocator_size = sizeof(Allocator);

    auto total_size =
        shared_queue_size + allocator_size + buffer_size + 3 * GAP;
    bool new_segment = false;

    auto *base_address = create_memory_segment(name, total_size, &new_segment);

    if (base_address == nullptr) {
      std::cerr << "Could not create shared memory buffer" << std::endl;
      exit(1);
    }

    auto *shared_queue_address = base_address;
    auto *allocator_address = shared_queue_address + shared_queue_size + GAP;
    auto *buffer_address = allocator_address + allocator_size + GAP;

    if (new_segment) {
      shared_queue_ =
          new (shared_queue_address) SharedQueue<ElemT, queue_size>();
      allocator_ = new (allocator_address)
          Allocator(buffer_address - allocator_address, buffer_size);
      init_shared_queue();
    } else {
      /*
       * We do a cast first. So if the shared queue isn't init'd
       * in time, i.e., `init_shared_queue` is still running,
       * we need to wait a bit to ensure nothing crazy happens.
       * The exact time to sleep is just a guess: depends on file IO.
       * Currently: 10ms.
       */
      shared_queue_ = reinterpret_cast<SharedQueue<ElemT, queue_size> *>(
          shared_queue_address);
      allocator_ = reinterpret_cast<Allocator *>(allocator_address);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  ~Memory() = default;

  void init_shared_queue() {
    /*
     * This is only called by the first process. It initializes the
     * counter to 0, and writes to name of the topic to tmp system.
     */
    shared_queue_->counter = 0;
    tmp::write(name_);
  }

  std::string name_;
  Allocator *allocator_;
  SharedQueue<ElemT, queue_size> *shared_queue_;
};

}  // namespace shm::memory

#endif  // INCLUDE_SHADESMAR_MEMORY_MEMORY_H_
