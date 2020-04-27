//
// Created by squadrick on 08/02/20.
//

// HERE BE DRAGONS

#ifndef __APPLE__ // no MaxOS support

#ifndef SHADESMAR_DRAGONS_H
#define SHADESMAR_DRAGONS_H

#include <immintrin.h>
#include <thread>
#include <vector>

namespace shm::memory::dragons {
static inline void *_rep_movsb(void *d, const void *s, size_t n) {
  // Slower than using regular `memcpy`
  asm volatile("rep movsb"
               : "=D"(d), "=S"(s), "=c"(n)
               : "0"(d), "1"(s), "2"(n)
               : "memory");
  return d;
}

static inline void *_avx_async_cpy(void *d, const void *s, size_t n) {
  //  assert(n % 32 == 0);
  //  assert((intptr_t(d) & 31) == 0);
  //  assert((intptr_t(s) & 31) == 0);
  //   d, s -> 32 byte aligned
  //   n -> multiple of 32

  auto *dVec = reinterpret_cast<__m256i *>(d);
  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  size_t nVec = n / sizeof(__m256i);
  for (; nVec > 0; nVec--, sVec++, dVec++) {
    const __m256i temp = _mm256_stream_load_si256(sVec);
    _mm256_stream_si256(dVec, temp);
  }
  _mm_sfence();
  return d;
}

void _async_stream_cpy(__m256i *d, const __m256i *s, size_t n) {
  for (; n > 0; n--, d++, s++) {
    const __m256i temp = _mm256_stream_load_si256(s);
    _mm256_stream_si256(d, temp);
  }
}

void *_multithread_avx_async_cpy(void *d, const void *s, size_t n) {
  const uint32_t maxThreads = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  threads.reserve(maxThreads);

  const auto *sVec = reinterpret_cast<const __m256i *>(s);
  auto *dVec = reinterpret_cast<__m256i *>(d);
  size_t nVec = n / sizeof(__m256i);

  lldiv_t perWorker = div((int64_t)nVec, maxThreads);

  size_t nextStart = 0;
  for (uint32_t threadIdx = 0; threadIdx < maxThreads; ++threadIdx) {
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
  return d;
}

void *_multithreaded_memcpy(void *d, const void *s, size_t n) {
  const uint32_t maxThreads = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  threads.reserve(maxThreads);

  lldiv_t perWorker = div((int64_t)n, maxThreads);

  size_t nextStart = 0;
  for (uint32_t threadIdx = 0; threadIdx < maxThreads; ++threadIdx) {
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
  return d;
}

} // namespace shm::memory::dragons
#endif // SHADESMAR_DRAGONS_H
#endif // __APPLE__
