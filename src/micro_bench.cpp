//
// Created by squadrick on 16/11/19.
//

#include <iostream>
#include <msgpack.hpp>

#include <shadesmar/macros.h>
#include <shadesmar/memory.h>
#include <shadesmar/message.h>

#include <shadesmar/ipc_lock.h>

const size_t mem_size = 10 * 1024 * 1024;

class TestMessage : shm::BaseMsg {
public:
  std::vector<uint32_t> arr;
  SHM_PACK(arr);

  TestMessage() = default;

  void create(int n) { arr = std::vector<uint32_t>(n); }
};

void bench_msgpack() {
  /*
   * Serialization, Deserialization time
   *  1kB = 2.5us, 17us
   *  1MB = 2.5ms, 17ms
   *  10MB = 25ms, 170ms
   */
  TestMessage t;
  t.create(mem_size);

  msgpack::sbuffer buf;
  msgpack::object_handle oh;

  TIMEIT({ msgpack::pack(buf, t); }, "serialize");

  msgpack::object_handle res;
  msgpack::object obj;
  TestMessage t_new;
  TIMEIT(
      {
        res = msgpack::unpack(buf.data(), buf.size());
        obj = res.get();
        obj.convert(t_new);
      },
      "deserialize");
}

void bench_memcpy() {
  /*
   * 1kb = 2.4us, 0.1us
   * 1MB = 800us, 650us
   * 10MB = 7ms, 6.5ms
   */

  bool new_segment;

  void *shm_mem = shm::create_memory_segment("test", mem_size, new_segment);
  void *obj = malloc(mem_size);
  TIMEIT({ std::memcpy(shm_mem, obj, mem_size); }, "obj->shm_mem");
  free(obj);
  shm_unlink("test");

  void *obj2 = malloc(mem_size);
  void *shm_mem2 = shm::create_memory_segment("test2", mem_size, new_segment);
  std::memset(shm_mem2, 0, mem_size);

  TIMEIT({ std::memcpy(obj2, shm_mem2, mem_size); }, "shm_mem->obj");
  free(obj2);
  shm_unlink("test2");
}

void bench_lock() {
  /*
   * All take ~1500ns
   * Peaks at ~100us when prune_sharable is called
   */
  shm::IPC_Lock lock;

  TIMEIT({ lock.lock(); }, "lock");
  TIMEIT({ lock.unlock(); }, "unlock");
  TIMEIT({ lock.lock_sharable(); }, "lock sharable");
  TIMEIT({ lock.unlock_sharable(); }, "unlock sharable");
}

void bench_new_lock() {
  /*
   * All take ~200ns
   */
  shm::IPC_Lock lock;

  TIMEIT({ lock.lock(); }, "lock (new)");
  TIMEIT({ lock.unlock(); }, "unlock (new)");
  TIMEIT({ lock.lock_sharable(); }, "lock sharable (new)");
  TIMEIT({ lock.unlock_sharable(); }, "unlock sharable (new)");
}

/*
 * Total time
 * 1kB = 6us, 18us
 * 1MB = 3.3ms, 18ms
 * 10MB = 33ms, 175ms (write: 30/s, read: 5/s)
 */
int main() {
  bench_msgpack();
  bench_memcpy();
  bench_lock();
  bench_new_lock();
}
