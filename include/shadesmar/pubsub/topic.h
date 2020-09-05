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

#ifndef INCLUDE_SHADESMAR_PUBSUB_TOPIC_H_
#define INCLUDE_SHADESMAR_PUBSUB_TOPIC_H_

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "shadesmar/concurrency/scope.h"
#include "shadesmar/macros.h"
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/memory.h"

namespace shm::pubsub {

template <class LockT>
struct TopicElemT : public memory::Element {
  LockT mutex;

  TopicElemT() : Element(), mutex() {}

  TopicElemT(const TopicElemT &topic_elem) : Element(topic_elem) {
    mutex = topic_elem.mutex;
  }
};

using LockType = concurrent::PthreadReadWriteLock;

class Topic {
  using TopicElem = TopicElemT<LockType>;

  template <concurrent::ExlOrShr type>
  using Scope = concurrent::ScopeGuard<LockType, type>;

 public:
  Topic(const std::string &topic, memory::Copier *copier) : memory_(topic) {
    if (copier == nullptr) {
      copier = new memory::DefaultCopier();
    }
    copier_ = copier;
  }

  bool write(memory::Memblock memblock) {
    /*
     * Writes always happen at the head of the circular queue, the
     * head is atomically incremented to prevent any race across
     * processes. The head of the queue is stored as a counter
     * on shared memory.
     */
    if (memblock.size > memory_.allocator_->get_free_memory()) {
      std::cerr << "Increase buffer_size" << std::endl;
      return false;
    }
    uint32_t q_pos = fetch_add_counter() & (queue_size() - 1);
    TopicElem *elem = &(memory_.shared_queue_->elements[q_pos]);

    /*
     * Code path:
     *  1. Allocate shared memory buffer `new_address`
     *  2. Copy msg data to `new_address`
     *  3. Acquire exclusive lock
     *  4. Complex "swap" of old and new fields
     *  5. Release exclusive lock
     *  6. If old buffer is empty, deallocate it
     */

    uint8_t *new_address = memory_.allocator_->alloc(memblock.size);
    if (new_address == nullptr) {
      return false;
    }

    copier_->user_to_shm(new_address, memblock.ptr, memblock.size);

    uint8_t *old_address = nullptr;
    {
      /*
       * This locked block should *only* contain accesses
       * to `elem`, any other expensive compute that doesn't
       * include `elem` can be put outside this block.
       */
      Scope<concurrent::EXCLUSIVE> _(&elem->mutex);
      if (!elem->empty) {
        old_address = memory_.allocator_->handle_to_ptr(elem->address_handle);
      }
      elem->address_handle = memory_.allocator_->ptr_to_handle(new_address);
      elem->size = memblock.size;
      elem->empty = false;
    }

    while (!memory_.allocator_->free(old_address)) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    return true;
  }

  /*
   * Reads aren't like writes in one major way: writes don't require
   * any information about which position in the queue to write to. It
   * defaults to the head of the queue. Reads requires an explicit
   * position in the queue to read(`pos`). We use this position
   * argument since we need to support multiple subscribers each
   * reading at their own pace. Look at `subscriber.h` to see the
   * logic to handle circular queue wrap around logic.
   */

  bool read(memory::Memblock *memblock, uint32_t pos) {
    /*
     * Read into a raw array.
     */
    TopicElem *elem =
        &(memory_.shared_queue_->elements[pos & (queue_size() - 1)]);

    /*
     * Code path:
     *  1. Acquire sharable lock
     *  2. Check for emptiness
     *  3. Copy from shared memory to input param `msg`
     *  4. Release sharable lock
     */
    Scope<concurrent::SHARED> _(&elem->mutex);

    if (elem->empty) {
      return false;
    }

    auto *dst = memory_.allocator_->handle_to_ptr(elem->address_handle);
    memblock->size = elem->size;
    memblock->ptr = copier_->alloc(memblock->size);

    copier_->shm_to_user(memblock->ptr, dst, memblock->size);

    return true;
  }

  inline __attribute__((always_inline)) uint32_t fetch_add_counter() const {
    return memory_.shared_queue_->counter.fetch_add(1);
  }

  inline __attribute__((always_inline)) uint32_t counter() const {
    return memory_.shared_queue_->counter.load();
  }

  size_t queue_size() const { return memory_.queue_size(); }

  inline memory::Copier *copier() const { return copier_; }

 private:
  memory::Memory<TopicElem> memory_;
  memory::Copier *copier_{};
};
}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_TOPIC_H_
