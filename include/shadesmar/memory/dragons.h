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

#include <immintrin.h>

#include <cstdlib>

#include <thread>
#include <vector>

#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/memory.h"

#ifndef __APPLE__  // no MaxOS support

namespace shm::memory::dragons {

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

static inline void _avx_cpy(void *d, const void *s, size_t n) {
  // d, s -> 32 byte aligned
  // n -> multiple of 32

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec--, sVec++, dVec++) {
    const __m256i temp = _mm256_load_si256(sVec);
    _mm256_store_si256(dVec, temp);
  }
}

class AvxCopier : public Copier {
 public:
  constexpr static size_t alignment = sizeof(__m256i);

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_cpy(dst, src, SHMALIGN(size, alignment));
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_cpy(dst, src, SHMALIGN(size, alignment));
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

static inline void _avx_async_pf_cpy(void *d, const void *s, size_t n) {
  // d, s -> 32 byte aligned
  // n -> multiple of 32

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 1; nVec--, sVec++, dVec++) {
    _mm_prefetch(sVec + 1, _MM_HINT_T0);
    const __m256i temp = _mm256_load_si256(sVec);
    _mm256_stream_si256(dVec, temp);
  }
  _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
  _mm_sfence();
}

class AvxAsyncPFCopier : public Copier {
 public:
  constexpr static size_t alignment = sizeof(__m256i);

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy(dst, src, SHMALIGN(size, alignment));
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy(dst, src, SHMALIGN(size, alignment));
  }
};

//------------------------------------------------------------------------------

static inline void _avx_unroll_cpy(void *d, const void *s, size_t n) {
  // d, s -> 128 byte aligned
  // n -> multiple of 128

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec -= 4, sVec += 4, dVec += 4) {
    _mm256_store_si256(dVec, _mm256_load_si256(sVec));
    _mm256_store_si256(dVec + 1, _mm256_load_si256(sVec + 1));
    _mm256_store_si256(dVec + 2, _mm256_load_si256(sVec + 2));
    _mm256_store_si256(dVec + 3, _mm256_load_si256(sVec + 3));
  }
}

class AvxUnrollCopier : public Copier {
 public:
  constexpr static size_t alignment = 4 * sizeof(__m256i);

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_unroll_cpy(dst, src, SHMALIGN(size, alignment));
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_unroll_cpy(dst, src, SHMALIGN(size, alignment));
  }
};

//------------------------------------------------------------------------------

}  // namespace shm::memory::dragons

#endif  // __APPLE__
#endif  // INCLUDE_SHADESMAR_MEMORY_DRAGONS_H_
