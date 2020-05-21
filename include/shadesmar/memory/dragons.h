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

// HERE BE DRAGONS

#ifndef INCLUDE_SHADESMAR_MEMORY_DRAGONS_H_
#define INCLUDE_SHADESMAR_MEMORY_DRAGONS_H_

#ifndef __APPLE__  // no MaxOS support

#include <immintrin.h>

#include <cstdlib>

#include <thread>
#include <vector>

#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/memory.h"

namespace shm::memory::dragons {

const uint32_t MAX_THREADS = std::thread::hardware_concurrency();

//------------------------------------------------------------------------------

static inline void _rep_movsb(void *d, const void *s, size_t n) {
  asm volatile("rep movsb"
               : "=D"(d), "=S"(s), "=c"(n)
               : "0"(d), "1"(s), "2"(n)
               : "memory");
}

class RepMovsbCopier : public Copier {
 public:
  void *alloc(size_t size) override { return malloc(size); }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _rep_movsb(dst, src, size);
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _rep_movsb(dst, src, size);
  }
};

//------------------------------------------------------------------------------

static inline void _avx_async_cpy(void *d, const void *s, size_t n) {
  // d, s -> 32 byte aligned
  // n -> multiple of 32

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec--, sVec++, dVec++) {
    const __m256i temp = _mm256_stream_load_si256(sVec);
    _mm256_stream_si256(dVec, temp);
  }
  _mm_sfence();
}

class AvxAsyncCopier : public Copier {
 public:
  constexpr static size_t alignment = sizeof(__m256i);

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_cpy(dst, src, SHMALIGN(size, alignment));
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_cpy(dst, src, SHMALIGN(size, alignment));
  }
};

//------------------------------------------------------------------------------

void _async_stream_cpy(__m256i *d, const __m256i *s, size_t n) {
  for (; n > 0; n--, d++, s++) {
    const __m256i temp = _mm256_stream_load_si256(s);
    _mm256_stream_si256(d, temp);
  }
}

void _multithread_avx_async_cpy(void *d, const void *s, size_t n,
                                uint32_t nthreads) {
  std::vector<std::thread> threads;
  threads.reserve(nthreads);

  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  auto *dVec = reinterpret_cast<__m256i *>(d);
  size_t nVec = n / sizeof(__m256i);

  ldiv_t perWorker = div((int64_t)nVec, nthreads);

  size_t nextStart = 0;
  for (uint32_t threadIdx = 0; threadIdx < nthreads; ++threadIdx) {
    const size_t currStart = nextStart;
    nextStart += perWorker.quot;
    if (threadIdx < perWorker.rem) {
      nextStart++;
    }
    threads.emplace_back(_async_stream_cpy, dVec + currStart, sVec + currStart,
                         nextStart - currStart);
  }
  for (auto &thread : threads) {
    thread.join();
  }
  _mm_sfence();
  threads.clear();
}

class MTAvxAsyncCopier : public Copier {
 public:
  constexpr static size_t alignment = sizeof(__m256i);

  explicit MTAvxAsyncCopier(uint32_t nthreads = MAX_THREADS)
      : threads(nthreads) {}
  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _multithread_avx_async_cpy(dst, src, SHMALIGN(size, alignment), threads);
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _multithread_avx_async_cpy(dst, src, SHMALIGN(size, alignment), threads);
  }

  uint32_t threads;
};

//------------------------------------------------------------------------------

static inline void _multithread_memcpy(void *d, void *s, size_t n,
                                       uint32_t nthreads) {
  std::vector<std::thread> threads;
  threads.reserve(nthreads);

  ldiv_t perWorker = div((int64_t)n, nthreads);

  size_t nextStart = 0;
  for (uint32_t threadIdx = 0; threadIdx < nthreads; ++threadIdx) {
    const size_t currStart = nextStart;
    nextStart += perWorker.quot;
    if (threadIdx < perWorker.rem) {
      nextStart++;
    }
    uint8_t *dTemp = reinterpret_cast<uint8_t *>(d) + currStart;
    const uint8_t *sTemp = reinterpret_cast<const uint8_t *>(s) + currStart;

    threads.emplace_back(memcpy, dTemp, sTemp, nextStart - currStart);
  }
  for (auto &thread : threads) {
    thread.join();
  }
  threads.clear();
}

class MTCopier : public Copier {
 public:
  const size_t alignment = 32;
  explicit MTCopier(uint32_t nthreads = MAX_THREADS) : threads(nthreads) {}

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _multithread_memcpy(dst, src, size, threads);
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _multithread_memcpy(dst, src, size, threads);
  }

  uint32_t threads;
};

//------------------------------------------------------------------------------

}  // namespace shm::memory::dragons

#endif  // __APPLE__
#endif  // INCLUDE_SHADESMAR_MEMORY_DRAGONS_H_
