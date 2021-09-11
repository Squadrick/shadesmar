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
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/memory.h"

namespace shm::pubsub {

template <class LockT>
struct ChannelElemT : public memory::Element {
  LockT mutex;

  ChannelElemT() : Element(), mutex() {}

  ChannelElemT(const ChannelElemT &topic_elem) : Element(topic_elem) {
    mutex = topic_elem.mutex;
  }
};

using LockType = concurrent::PthreadReadWriteLock;

// clients are calling RPCs provided by servers.
class Channel {
  using ChannelElem = ChannelElemT<LockType>;

  template <concurrent::ExlOrShr type>
  using Scope = concurrent::ScopeGuard<LockType, type>;

 public:
  Channel(const std::string &channel, memory::Copier *copier)
      : memory_(channel) {
    if (copier == nullptr) {
      copier = new memory::DefaultCopier();
    }
    copier_ = copier;
  }

  ~Channel() {
    if (copier_) {
      delete copier_;
    }
  }

  /*
   * client->call()
   * write_client()
   * read_server()
   * RPC()
   * write_server()
   * read_client()
   */

  bool write_client(memory::Memblock memblock, uint32_t *pos) { return false; }

  bool read_client(uint32_t pos, memory::Memblock *memblock) { return false; }

  bool write_server(memory::Memblock memblock, uint32_t pos) { return false; }

  bool read_server(memory::Memblock *memblock, uint32_t *pos) { return false; }

  inline __attribute__((always_inline)) void inc_counter() {
    memory_.shared_queue_->counter++;
  }

  inline __attribute__((always_inline)) uint32_t counter() const {
    return memory_.shared_queue_->counter.load();
  }

  size_t queue_size() const { return memory_.queue_size(); }

  inline memory::Copier *copier() const { return copier_; }

 private:
  memory::Memory<ChannelElem> memory_;
  memory::Copier *copier_;
};

}  // namespace shm::pubsub

#endif  // INCLUDE_SHADESMAR_RPC_CHANNEL_H_
