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

#include "shadesmar/concurrency/cond_var.h"
#include "shadesmar/concurrency/lock.h"
#include "shadesmar/concurrency/scope.h"
#include "shadesmar/macros.h"
#include "shadesmar/memory/allocator.h"
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/double_allocator.h"
#include "shadesmar/memory/memory.h"

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

// clients are calling RPCs provided by servers.
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
      inc_counter();  // move this after successful alloc? It'll prevent empty
                      // req cells.
      elem->req.empty = false;
    }

    // With the code-block, when a thread reaches here, it is the only one that
    // can access `elem`. No need for lock.

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
      // This is how we convey an error: We mark that the result is written,
      // so `read_client` will still read this value, but since the address
      // handle for the result is 0, it can't read from buffer.
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

}  // namespace shm::rpc

#endif  // INCLUDE_SHADESMAR_RPC_CHANNEL_H_
