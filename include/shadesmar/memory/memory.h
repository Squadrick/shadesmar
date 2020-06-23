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

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>

#include <atomic>
#include <memory>
#include <string>
#include <type_traits>

#include <boost/interprocess/managed_shared_memory.hpp>

#include "shadesmar/concurrency/robust_lock.h"
#include "shadesmar/macros.h"
#include "shadesmar/memory/allocator.h"
#include "shadesmar/memory/tmp.h"

using managed_shared_memory = boost::interprocess::managed_shared_memory;

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

struct Ptr {
  void *ptr;
  size_t size;
  bool free;

  Ptr() : ptr(nullptr), size(0), free(false) {}

  void no_delete() { free = false; }
};

struct Element {
  size_t size;
  std::atomic<bool> empty{};
  managed_shared_memory::handle_t addr_hdl;

  Element() : size(0), addr_hdl(0) { empty.store(true); }

  Element(const Element &elem) : size(elem.size), addr_hdl(elem.addr_hdl) {
    empty.store(elem.empty.load());
  }
};

template <class ElemT, uint32_t queue_size>
class SharedQueue {
 public:
  std::atomic<uint32_t> counter;
  std::array<ElemT, queue_size> elements;
};

static size_t max_buffer_size = (1U << 28);  // 256mb

template <class ElemT, uint32_t queue_size = 1>
class Memory {
  static_assert(std::is_base_of<Element, ElemT>::value,
                "ElemT must be a subclass of Element");

  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

 public:
  explicit Memory(const std::string &name) : name_(name) {
    bool new_segment;
    auto *base_addr = create_memory_segment(
        name, sizeof(SharedQueue<ElemT, queue_size>), &new_segment);

    if (base_addr == nullptr) {
      std::cerr << "Could not create shared memory buffer" << std::endl;
      exit(1);
    }

    raw_buf_ = std::make_shared<managed_shared_memory>(
        boost::interprocess::open_or_create, (name + "Raw").c_str(),
        max_buffer_size);

    if (new_segment) {
      shared_queue_ = new (base_addr) SharedQueue<ElemT, queue_size>();
      init_shared_queue();
    } else {
      /*
       * We do a cast first. So if the shared queue isn't init'd
       * in time, i.e., `init_shared_queue` is still running,
       * we need to wait a bit to ensure nothing crazy happens.
       * The exact time to sleep is just a guess: depends on file IO.
       * Currently: 10ms.
       */
      shared_queue_ =
          reinterpret_cast<SharedQueue<ElemT, queue_size> *>(base_addr);
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
  std::shared_ptr<managed_shared_memory> raw_buf_;
  SharedQueue<ElemT, queue_size> *shared_queue_;
};

}  // namespace shm::memory

#endif  // INCLUDE_SHADESMAR_MEMORY_MEMORY_H_
