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
  auto *alloc =
      new (memory) shm::memory::Allocator(sizeof(shm::memory::Allocator), size);
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

void multithread(int nthreads) {
  auto *alloc = new_alloc(128 * nthreads);

  std::vector<std::thread> threads;
  threads.reserve(nthreads);
  for (int t = 0; t < nthreads; ++t) {
    threads.emplace_back([&]() {
      int iters = 100;
      while (iters--) {
        auto *p1 = alloc->alloc(16);
        auto *p2 = alloc->alloc(10);

        assert(p1 != nullptr);
        assert(p2 != nullptr);

        int max_tries = nthreads * nthreads;

        auto timed_free_loop = [&](uint8_t *p) -> bool {
          for (int i = 0; i < max_tries; ++i) {
            if (alloc->free(p)) {
              return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
          }
          return false;
        };

        assert(timed_free_loop(p1));
        assert(timed_free_loop(p2));
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

  multithread(128);  // FLAKY
}
