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

namespace shm::memory::dragons {

//------------------------------------------------------------------------------

#ifdef __x86_64__

static inline void _rep_movsb(void *d, const void *s, size_t n) {
  asm volatile("rep movsb"
               : "=D"(d), "=S"(s), "=c"(n)
               : "0"(d), "1"(s), "2"(n)
               : "memory");
}

class RepMovsbCopier : public Copier {
 public:
  using PtrT = uint8_t;
  void *alloc(size_t size) override { return malloc(size); }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _rep_movsb(dst, src, size);
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _rep_movsb(dst, src, size);
  }
};

#endif  // __x86_64__

//------------------------------------------------------------------------------

#ifdef __AVX__

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
  using PtrT = __m256i;
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

#endif  // __AVX__

//------------------------------------------------------------------------------

#ifdef __AVX2__

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
  using PtrT = __m256i;
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

#endif  // __AVX2__

//------------------------------------------------------------------------------

#ifdef __AVX2__

static inline void _avx_async_pf_cpy(void *d, const void *s, size_t n) {
  // d, s -> 64 byte aligned
  // n -> multiple of 64

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 2; nVec -= 2, sVec += 2, dVec += 2) {
    // prefetch the next iteration's data
    // by default _mm_prefetch moves the entire cache-lint (64b)
    _mm_prefetch(sVec + 2, _MM_HINT_T0);

    _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
    _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
  }
  _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
  _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
  _mm_sfence();
}

class AvxAsyncPFCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = sizeof(__m256i) * 2;

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

#endif  // __AVX2__

//------------------------------------------------------------------------------

#ifdef __AVX__

static inline void _avx_cpy_unroll(void *d, const void *s, size_t n) {
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
  using PtrT = __m256i;
  constexpr static size_t alignment = 4 * sizeof(__m256i);

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
};

#endif  // __AVX__

//------------------------------------------------------------------------------

#ifdef __AVX2__

static inline void _avx_async_cpy_unroll(void *d, const void *s, size_t n) {
  // d, s -> 128 byte aligned
  // n -> multiple of 128

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec -= 4, sVec += 4, dVec += 4) {
    _mm256_stream_si256(dVec, _mm256_stream_load_si256(sVec));
    _mm256_stream_si256(dVec + 1, _mm256_stream_load_si256(sVec + 1));
    _mm256_stream_si256(dVec + 2, _mm256_stream_load_si256(sVec + 2));
    _mm256_stream_si256(dVec + 3, _mm256_stream_load_si256(sVec + 3));
  }
  _mm_sfence();
}

class AvxAsyncUnrollCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = 4 * sizeof(__m256i);

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
};

#endif  // __AVX2__

//------------------------------------------------------------------------------

#ifdef __AVX2__

static inline void _avx_async_pf_cpy_unroll(void *d, const void *s, size_t n) {
  // d, s -> 128 byte aligned
  // n -> multiple of 128

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 4; nVec -= 4, sVec += 4, dVec += 4) {
    // prefetch data for next iteration
    _mm_prefetch(sVec + 4, _MM_HINT_T0);
    _mm_prefetch(sVec + 6, _MM_HINT_T0);
    _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
    _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
    _mm256_stream_si256(dVec + 2, _mm256_load_si256(sVec + 2));
    _mm256_stream_si256(dVec + 3, _mm256_load_si256(sVec + 3));
  }
  _mm256_stream_si256(dVec, _mm256_load_si256(sVec));
  _mm256_stream_si256(dVec + 1, _mm256_load_si256(sVec + 1));
  _mm256_stream_si256(dVec + 2, _mm256_load_si256(sVec + 2));
  _mm256_stream_si256(dVec + 3, _mm256_load_si256(sVec + 3));
  _mm_sfence();
}

class AvxAsyncPFUnrollCopier : public Copier {
 public:
  using PtrT = __m256i;
  constexpr static size_t alignment = 4 * sizeof(__m256i);

  void *alloc(size_t size) override {
    return aligned_alloc(alignment, SHMALIGN(size, alignment));
  }

  void dealloc(void *ptr) override { free(ptr); }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _avx_async_pf_cpy_unroll(dst, src, SHMALIGN(size, alignment));
  }
};

#endif  // __AVX2__

//------------------------------------------------------------------------------

template <class BaseCopierT, uint32_t nthreads>
class MTCopier : public Copier {
 public:
  MTCopier() : base_copier() {}

  void *alloc(size_t size) override { return base_copier.alloc(size); }

  void dealloc(void *ptr) override { base_copier.dealloc(ptr); }

  void _copy(void *d, void *s, size_t n, bool shm_to_user) {
    n = SHMALIGN(n, sizeof(typename BaseCopierT::PtrT)) /
        sizeof(typename BaseCopierT::PtrT);
    std::vector<std::thread> threads;
    threads.reserve(nthreads);

    auto per_worker = div((int64_t)n, nthreads);

    size_t next_start = 0;
    for (uint32_t thread_idx = 0; thread_idx < nthreads; ++thread_idx) {
      const size_t curr_start = next_start;
      next_start += per_worker.quot;
      if (thread_idx < per_worker.rem) {
        ++next_start;
      }
      auto d_thread =
          reinterpret_cast<typename BaseCopierT::PtrT *>(d) + curr_start;
      auto s_thread =
          reinterpret_cast<typename BaseCopierT::PtrT *>(s) + curr_start;

      if (shm_to_user) {
        threads.emplace_back(
            &Copier::shm_to_user, &base_copier, d_thread, s_thread,
            (next_start - curr_start) * sizeof(typename BaseCopierT::PtrT));
      } else {
        threads.emplace_back(
            &Copier::user_to_shm, &base_copier, d_thread, s_thread,
            (next_start - curr_start) * sizeof(typename BaseCopierT::PtrT));
      }
    }
    for (auto &thread : threads) {
      thread.join();
    }
    threads.clear();
  }

  void shm_to_user(void *dst, void *src, size_t size) override {
    _copy(dst, src, size, true);
  }

  void user_to_shm(void *dst, void *src, size_t size) override {
    _copy(dst, src, size, false);
  }

 private:
  BaseCopierT base_copier;
};

//------------------------------------------------------------------------------

}  // namespace shm::memory::dragons

#endif  // INCLUDE_SHADESMAR_MEMORY_DRAGONS_H_
