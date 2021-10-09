/* MIT License

Copyright (c) 2021 Dheeraj R Reddy

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

#ifndef INCLUDE_SHADESMAR_RPC_CHANNEL_H_
#define INCLUDE_SHADESMAR_RPC_CHANNEL_H_

#include <atomic>
#include <memory>
#include <string>

#include "shadesmar/concurrency/scope.h"
#include "shadesmar/macros.h"
#include "shadesmar/memory/allocator.h"
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/double_allocator.h"
#include "shadesmar/memory/memory.h"

namespace shm::rpc {

template <class LockT>
struct ChannelElemT : public memory::Element {
  // TODO(squadrick): Find how to order these best.
  bool result_written;
  memory::Allocator::handle result_address_handle;
  LockT mutex;
  // add condvar here too.

  ChannelElemT()
      : Element(), mutex(), result_written(false), result_address_handle(0) {}

  ChannelElemT(const ChannelElemT &topic_elem) : Element(topic_elem) {
    mutex = topic_elem.mutex;
    result_address_handle = topic_elem.result_address_handle;
    result_written = topic_elem.result_written;
  }
};

using LockType = concurrent::PthreadReadWriteLock;

// clients are calling RPCs provided by servers.
class Channel {
  using ChannelElem = ChannelElemT<LockType>;

  template <concurrent::ExlOrShr type>
  using Scope = concurrent::ScopeGuard<LockType, type>;

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

  /*
   * client->call()
   * write_client()
   * read_server()
   * RPC()
   * write_server()
   * read_client()
   */

  bool write_client(memory::Memblock memblock, uint32_t *pos) {
    if (memblock.size > memory_.allocator_->req.get_free_memory()) {
      std::cerr << "Increase buffer_size" << std::endl;
      return false;
    }
    uint32_t q_pos = counter() & (queue_size() - 1);
    ChannelElem *elem = &(memory_.shared_queue_->elements[q_pos]);

    /*
     * Code path:
     *  1. Allocate shared memory buffer `new_address`
     *  2. Copy msg data to `new_address`
     *  3. Acquire exclusive lock
     *  4. Complex "swap" of old and new fields
     *
     */

    {
      Scope<concurrent::EXCLUSIVE> _(&elem->mutex);
      if (!elem->empty) {
        std::cerr << "Queue is full, try again." << std::endl;
        return false;
      }
      inc_counter();  // move this after successful alloc? It'll prevent empty
                      // req cells.
      elem->empty = false;
    }

    uint8_t *new_address = memory_.allocator_->req.alloc(memblock.size);
    if (new_address == nullptr) {
      elem->empty = true;
      return false;
    }

    copier_->user_to_shm(new_address, memblock.ptr, memblock.size);
    {
      Scope<concurrent::EXCLUSIVE> _(&elem->mutex);  // redundant?
      elem->address_handle =
          memory_.allocator_->req.ptr_to_handle(new_address);
      elem->size = memblock.size;
    }
    return true;
  }

  bool read_client(uint32_t pos, memory::Memblock *memblock) { return false; }

  bool write_server(memory::Memblock memblock, uint32_t pos) { return false; }

  bool read_server(uint32_t pos, memory::Memblock *memblock) {
    uint32_t q_pos = pos & (queue_size() - 1);
    ChannelElem *elem = &(memory_.shared_queue_->elements[q_pos]);
    Scope<concurrent::EXCLUSIVE> _(&elem->mutex);
    // TODO(squadrick): Use MOVE_ELEM from topic.h here.
    if (elem->empty) {
      return false;
    }
    uint8_t *address =
        memory_.allocator_->req.handle_to_ptr(elem->address_handle);
    memblock->size = elem->size;
    memblock->ptr = copier_->alloc(memblock->size);
    copier_->shm_to_user(memblock->ptr, address, memblock->size);
    return true;
  }

  // TODO(squadrick): Create an abstract class called Carrier with below
  // params, parameterized on ElementT and AllocatorT, and extend Topic/Channel
  // from this abstract class.
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

}  // namespace shm::rpc

#endif  // INCLUDE_SHADESMAR_RPC_CHANNEL_H_
