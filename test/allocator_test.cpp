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

#include <cstdlib>
#include <cstring>
#include <numeric>
#include <thread>
#include <vector>

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/concurrency/lock.h"
#include "shadesmar/memory/allocator.h"
#endif

shm::memory::Allocator *new_alloc(size_t size) {
  auto *memory = malloc(size + sizeof(shm::memory::Allocator));
  auto *alloc = new (memory)
      shm::memory::Allocator(sizeof(shm::memory::Allocator), size);
  return alloc;
}

void basic() {
  auto *alloc = new_alloc(1024 * 1024);

  auto *x = alloc->alloc(100);
  auto *y = alloc->alloc(250);
  auto *z = alloc->alloc(1000);

  auto xh = alloc->ptr_to_handle(x);
  auto yh = alloc->ptr_to_handle(y);
  auto zh = alloc->ptr_to_handle(z);

  assert(yh - xh > 100);
  assert(zh - yh > 250);

  assert(!alloc->free(z));
  assert(!alloc->free(y));
  assert(alloc->free(x));

  assert(!alloc->free(z));
  assert(alloc->free(y));
  assert(alloc->free(z));

  free(alloc);
}

void size_limit() {
  auto *alloc = new_alloc(100);

  auto *x = alloc->alloc(50);
  auto *y = alloc->alloc(32);
  auto *z = alloc->alloc(50);

  assert(x != nullptr);
  assert(y != nullptr);
  assert(z == nullptr);

  free(alloc);
}

void perfect_wrap_around() {
  auto *alloc = new_alloc(100);

  auto *x = alloc->alloc(50);
  auto *y = alloc->alloc(32);
  auto *z = alloc->alloc(50);

  assert(x != nullptr);
  assert(y != nullptr);
  assert(z == nullptr);

  assert(alloc->free(x));
  assert(alloc->free(y));

  z = alloc->alloc(50);
  assert(z != nullptr);

  free(alloc);
}

void wrap_around() {
  auto *alloc = new_alloc(100);

  auto *x = alloc->alloc(50);
  auto *y = alloc->alloc(32);

  assert(x != nullptr);
  assert(y != nullptr);

  assert(alloc->free(x));

  auto *z = alloc->alloc(40);
  assert(z != nullptr);

  assert(alloc->free(y));
  assert(alloc->free(z));

  free(alloc);
}

void cyclic() {
  auto *alloc = new_alloc(256);

  auto *it1 = alloc->alloc(40);
  auto *it2 = alloc->alloc(40);

  assert(it1 != nullptr);
  assert(it2 != nullptr);

  int iterations = 100;
  while (iterations--) {
    auto *it3 = alloc->alloc(40);
    auto *it4 = alloc->alloc(40);

    assert(it3 != nullptr);
    assert(it4 != nullptr);

    assert(alloc->free(it1));
    assert(alloc->free(it2));

    it1 = it3;
    it2 = it4;
  }

  assert(alloc->free(it1));
  assert(alloc->free(it2));

  free(alloc);
}

void multithread(int nthreads, std::vector<int> &&allocs) {
  auto *alloc = new_alloc(
      2 * std::accumulate(allocs.begin(), allocs.end(), 0) * nthreads);

  std::vector<std::thread> threads;
  threads.reserve(nthreads);
  for (int t = 0; t < nthreads; ++t) {
    auto rand_sleep = [&t](uint32_t factor) {
      uint32_t seed = t;
      // Sleep between (0, factor) microseconds
      // Seeded per thread.
      uint32_t timeout =
          static_cast<float>(rand_r(&seed) / static_cast<float>(RAND_MAX)) *
          factor;
      std::this_thread::sleep_for(std::chrono::microseconds(timeout));
    };

    threads.emplace_back([&, t]() {
      int iters = 100;
      while (iters--) {
        std::vector<uint8_t *> ps(allocs.size());

        for (int i = 0; i < allocs.size(); ++i) {
          ps[i] = alloc->alloc(allocs[i]);
          rand_sleep(10);
        }

        for (int i = 0; i < ps.size(); ++i) {
          assert(ps[i] != nullptr);
          std::memset(ps[i], ps.size() * t + i, allocs[i]);
        }

        rand_sleep(100);

        for (int i = 0; i < ps.size(); ++i) {
          for (int l = 0; l < allocs[i]; ++i) {
            assert(ps[i][l] == static_cast<uint8_t>(ps.size() * t + i));
          }
        }

        int max_tries = std::max(nthreads * nthreads, 100);
        auto timed_free_loop = [&](uint8_t *p) -> bool {
          for (int i = 0; i < max_tries; ++i) {
            if (alloc->free(p)) {
              return true;
            }
            rand_sleep(50);
          }
          return false;
        };

        for (auto p : ps) {
          assert(timed_free_loop(p));
        }
      }
    });
  }
  for (int t = 0; t < nthreads; ++t) {
    threads[t].join();
  }

  free(alloc);
}

int main() {
  basic();
  size_limit();
  perfect_wrap_around();
  wrap_around();
  cyclic();

  multithread(128, {100, 2000, 30000});
}
