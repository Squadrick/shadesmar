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
#include "shadesmar/memory/allocator.h"
#endif

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

shm::memory::Allocator *new_alloc(size_t size) {
  auto *memory = malloc(size + sizeof(shm::memory::Allocator));
  auto *alloc = new (memory)
      shm::memory::Allocator(sizeof(shm::memory::Allocator), size);
  return alloc;
}

TEST_CASE("basic") {
  auto *alloc = new_alloc(1024 * 1024);

  auto *x = alloc->alloc(100);
  auto *y = alloc->alloc(250);
  auto *z = alloc->alloc(1000);

  auto xh = alloc->ptr_to_handle(x);
  auto yh = alloc->ptr_to_handle(y);
  auto zh = alloc->ptr_to_handle(z);

  REQUIRE(yh - xh > 100);
  REQUIRE(zh - yh > 250);

  REQUIRE(!alloc->free(z));
  REQUIRE(!alloc->free(y));
  REQUIRE(alloc->free(x));

  REQUIRE(!alloc->free(z));
  REQUIRE(alloc->free(y));
  REQUIRE(alloc->free(z));

  free(alloc);
}

TEST_CASE("size_limit") {
  auto *alloc = new_alloc(100);

  auto *x = alloc->alloc(50);
  auto *y = alloc->alloc(32);
  auto *z = alloc->alloc(50);

  REQUIRE(x != nullptr);
  REQUIRE(y != nullptr);
  REQUIRE(z == nullptr);

  free(alloc);
}

TEST_CASE("perfect_wrap_around") {
  auto *alloc = new_alloc(100);

  auto *x = alloc->alloc(50);
  auto *y = alloc->alloc(32);
  auto *z = alloc->alloc(50);

  REQUIRE(x != nullptr);
  REQUIRE(y != nullptr);
  REQUIRE(z == nullptr);

  REQUIRE(alloc->free(x));
  REQUIRE(alloc->free(y));

  z = alloc->alloc(50);
  REQUIRE(z != nullptr);

  free(alloc);
}

TEST_CASE("wrap_around") {
  auto *alloc = new_alloc(100);

  auto *x = alloc->alloc(50);
  auto *y = alloc->alloc(32);

  REQUIRE(x != nullptr);
  REQUIRE(y != nullptr);

  REQUIRE(alloc->free(x));

  auto *z = alloc->alloc(40);
  REQUIRE(z != nullptr);

  REQUIRE(alloc->free(y));
  REQUIRE(alloc->free(z));

  free(alloc);
}

TEST_CASE("cyclic") {
  auto *alloc = new_alloc(256);

  auto *it1 = alloc->alloc(40);
  auto *it2 = alloc->alloc(40);

  REQUIRE(it1 != nullptr);
  REQUIRE(it2 != nullptr);

  int iterations = 100;
  while (iterations--) {
    auto *it3 = alloc->alloc(40);
    auto *it4 = alloc->alloc(40);

    REQUIRE(it3 != nullptr);
    REQUIRE(it4 != nullptr);

    REQUIRE(alloc->free(it1));
    REQUIRE(alloc->free(it2));

    it1 = it3;
    it2 = it4;
  }

  REQUIRE(alloc->free(it1));
  REQUIRE(alloc->free(it2));

  free(alloc);
}

TEST_CASE("multithread", "[!mayfail]") {
  int nthreads = 32;
  std::vector<int> allocs = {10, 200, 3000};
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
          REQUIRE(ps[i] != nullptr);
          std::memset(ps[i], ps.size() * t + i, allocs[i]);
        }

        rand_sleep(100);

        for (int i = 0; i < ps.size(); ++i) {
          for (int l = 0; l < allocs[i]; ++i) {
            REQUIRE(ps[i][l] == static_cast<uint8_t>(ps.size() * t + i));
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
          REQUIRE(timed_free_loop(p));
        }
      }
    });
  }
  for (int t = 0; t < nthreads; ++t) {
    threads[t].join();
  }

  free(alloc);
}
