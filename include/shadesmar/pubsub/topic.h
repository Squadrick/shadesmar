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

#include <cstring>

#include <atomic>
#include <iostream>
#include <memory>
#include <string>

#include <msgpack.hpp>

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

template <uint32_t queue_size = 1,
          class LockT = concurrent::PthreadReadWriteLock>
class Topic : public memory::Memory<TopicElemT<LockT>, queue_size> {
  using TopicElem = TopicElemT<LockT>;

  template <concurrent::ExlOrShr type>
  using Scope = concurrent::ScopeGuard<LockT, type>;

  static_assert((queue_size & (queue_size - 1)) == 0,
                "queue_size must be power of two");

 public:
  Topic(const std::string &topic, memory::Copier *copier, bool copy = false)
      : memory::Memory<TopicElem, queue_size>(topic),
        copy_(copy),
        copier_(copier) {}

  explicit Topic(const std::string &topic, bool copy = false)
      : Topic(topic, nullptr, copy) {}

  bool write(void *data, size_t size) {
    /*
     * Writes always happen at the head of the circular queue, the
     * head is atomically incremented to prevent any race across
     * processes. The head of the queue is stored as a counter
     * on shared memory.
     */
    if (size > this->allocator_->get_free_memory()) {
      std::cerr << "Increase buffer_size" << std::endl;
      return false;
    }
    uint32_t q_pos = fetch_add_counter() & (queue_size - 1);
    TopicElem *elem = &(this->shared_queue_->elements[q_pos]);

    /*
     * Code path:
     *  1. Allocate shared memory buffer `new_address`
     *  2. Copy msg data to `new_address`
     *  3. Acquire exclusive lock
     *  4. Complex "swap" of old and new fields
     *  5. Release exclusive lock
     *  6. If old buffer is empty, deallocate it
     */

    uint8_t *old_address = nullptr;
    uint8_t *new_address = this->allocator_->alloc(size);
    if (new_address == nullptr) {
      return false;
    }

    _copy(new_address, data, size);

    {
      /*
       * This locked block should *only* contain accesses
       * to `elem`, any other expensive compute that doesn't
       * include `elem` can be put outside this block.
       */
      Scope<concurrent::EXCLUSIVE> _(&elem->mutex);
      if (!elem->empty) {
        old_address = this->allocator_->handle_to_ptr(elem->address_handle);
      }
      elem->address_handle = this->allocator_->ptr_to_handle(new_address);
      elem->size = size;
      elem->empty = false;
    }

    if (old_address != nullptr) {
      while (!this->allocator_->free(old_address)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
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

  bool read(msgpack::object_handle *oh, uint32_t pos) {
    /*
     * Read directly into object handle.
     */
    TopicElem *elem = &(this->shared_queue_->elements[pos & (queue_size - 1)]);

    if (copy_) {
      return _read_with_copy(oh, elem);
    } else {
      return _read_without_copy(oh, elem);
    }
  }

  bool read(memory::Ptr *ptr, uint32_t pos) {
    /*
     * Read into a raw array. We pass `size` as a reference to store
     * the size of the message.
     */
    TopicElem *elem = &(this->shared_queue_->elements[pos & (queue_size - 1)]);
    return _read_bin(ptr, elem);
  }

  inline __attribute__((always_inline)) uint32_t fetch_add_counter() {
    return this->shared_queue_->counter.fetch_add(1);
  }

  inline __attribute__((always_inline)) uint32_t counter() {
    return this->shared_queue_->counter.load();
  }

  inline __attribute__((always_inline)) void inc_counter() {
    this->shared_queue_->counter.fetch_add(1);
  }

  inline memory::Copier *copier() const { return copier_; }

 private:
  bool _read_without_copy(msgpack::object_handle *oh, TopicElem *elem) {
    /*
     * Code path:
     *  1. Acquire sharable lock
     *  2. Check for emptiness
     *  3. Unpack shared memory into the object handle
     *  4. Release sharable lock
     */
    Scope<concurrent::SHARED> _(&elem->mutex);

    if (elem->empty) {
      return false;
    }

    const char *dst = reinterpret_cast<const char *>(
        this->allocator_->handle_to_ptr(elem->address_handle));

    *oh = msgpack::unpack(dst, elem->size);
    return true;
  }

  bool _read_with_copy(msgpack::object_handle *oh, TopicElem *elem) {
    /*
     * Code path:
     *  1. Create temporary buffer
     *  2. Safe copy from shared memory to tmp buffer (`_read_bin`)
     *  3. Unpack temporary buffer into the object handle
     *
     * Can't check for `elem->empty` here since we don't have a
     * sharable lock. We can't have a sharable lock, since `_read_bin`
     * also acquires a sharable lock, and it'll become recursive.
     * So we bite the cost of the extra function call before checking
     * for emptiness.
     */

    memory::Ptr ptr;

    if (_read_bin(&ptr, elem)) {
      *oh = msgpack::unpack(reinterpret_cast<const char *>(ptr.ptr), ptr.size);
      free(ptr.ptr);
      return true;
    }

    return false;
  }

  bool _read_bin(memory::Ptr *ptr, TopicElem *elem) {
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

    auto *dst = this->allocator_->handle_to_ptr(elem->address_handle);
    ptr->size = elem->size;
    ptr->ptr = _alloc(ptr->size);
    _copy(ptr->ptr, dst, ptr->size);

    return true;
  }

  void _copy(void *dst, void *src, size_t size) {
    if (copier_ == nullptr) {
      std::memcpy(dst, src, size);
    } else {
      copier_->user_to_shm(dst, src, size);
    }
  }

  void *_alloc(size_t size) {
    void *ptr = nullptr;
    if (copier_ != nullptr) {
      ptr = copier_->alloc(size);
    } else {
      ptr = malloc(size);
    }
    return ptr;
  }

  memory::Copier *copier_{};
  bool copy_{};
};
}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_TOPIC_H_
