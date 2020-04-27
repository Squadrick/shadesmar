//
// Created by squadrick on 27/11/19.
//

#ifndef shadesmar_MEMORY_H
#define shadesmar_MEMORY_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdint>

#include <atomic>
#include <type_traits>

#include <boost/interprocess/managed_shared_memory.hpp>

#include <shadesmar/concurrency/robust_lock.h>
#include <shadesmar/macros.h>
#include <shadesmar/memory/tmp.h>

using namespace boost::interprocess;

namespace shm::memory {

uint8_t *create_memory_segment(const std::string &name, size_t size,
                               bool &new_segment) {
  /*
   * Create a new shared memory segment. The segment is created
   * under a name. We check if an existing segment is found under
   * the same name. This info is stored in `new_segment`.
   *
   * The permission of the memory segment is 0644.
   */
  int fd;
  while (true) {
    new_segment = true;
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
      new_segment = false;
    }
    break;
  }
  int result = ftruncate(fd, size);
  if (result == EINVAL) {
    return nullptr;
  }

  auto ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  return static_cast<uint8_t *>(ptr);
}

struct Element {
  size_t size;
  std::atomic<bool> empty{};
  managed_shared_memory::handle_t addr_hdl;

  Element() {
    size = 0;
    empty.store(true);
    addr_hdl = 0;
  }

  Element(const Element &elem) {
    size = elem.size;
    empty.store(elem.empty.load());
    addr_hdl = elem.addr_hdl;
  }
};

template <class ElemT, uint32_t queue_size> class SharedQueue {
public:
  std::atomic<uint32_t> counter;
  std::array<ElemT, queue_size> elements;
};

static size_t max_buffer_size = (1U << 28); // 256mb

template <class ElemT, uint32_t queue_size = 1> class Memory {
  static_assert(std::is_base_of<Element, ElemT>::value,
                "ElemT must be a subclass of Element");

  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

public:
  explicit Memory(const std::string &name) : name_(name) {
    bool new_segment;
    auto *base_addr = create_memory_segment(
        name, sizeof(SharedQueue<ElemT, queue_size>), new_segment);

    if (base_addr == nullptr) {
      std::cerr << "Could not create shared memory buffer" << std::endl;
      exit(1);
    }

    raw_buf_ = std::make_shared<managed_shared_memory>(
        open_or_create, (name + "Raw").c_str(), max_buffer_size);

    if (new_segment) {
      shared_queue_ = new (base_addr) SharedQueue<ElemT, queue_size>();
      init_shared_queue();
    } else {
      /*
       * We do a cast first. So if the shared queue isn't init'd
       * in time, i.e., `init_shared_queue` is still running,
       * we need to wait a bit to ensure nothing crazy happens.
       * The exact time to sleep is just a guess: depends on file IO.
       * Currently: 100ms.
       */
      shared_queue_ =
          reinterpret_cast<SharedQueue<ElemT, queue_size> *>(base_addr);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

} // namespace shm::memory

#endif // shadesmar_MEMORY_H
