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
#include "shadesmar/memory/dragons.h"

#include <benchmark/benchmark.h>

#include <cstring>

#define STARTRANGE 1 << 5  // 32 bytes
#define ENDRANGE 1 << 23   // 8 MB
#define SHMRANGE STARTRANGE, ENDRANGE

template <class CopierT>
void CopyBench(benchmark::State &state) {  // NOLINT
  CopierT cpy;
  size_t size = state.range(0);
  auto *src = cpy.alloc(size);
  auto *dst = cpy.alloc(size);
  std::memset(src, 'x', size);
  for (auto _ : state) {
    cpy.shm_to_user(dst, src, size);
    benchmark::DoNotOptimize(dst);
  }
  cpy.dealloc(src);
  cpy.dealloc(dst);
  state.SetBytesProcessed(size * static_cast<int64_t>(state.iterations()));
}

#define SHM_COPY_BENCHMARK(c) \
  BENCHMARK_TEMPLATE(CopyBench, c)->Range(SHMRANGE);

SHM_COPY_BENCHMARK(shm::memory::DefaultCopier);
#ifdef __x86_64__
SHM_COPY_BENCHMARK(shm::memory::dragons::RepMovsbCopier);
#endif
#ifdef __AVX__
SHM_COPY_BENCHMARK(shm::memory::dragons::AvxCopier);
SHM_COPY_BENCHMARK(shm::memory::dragons::AvxUnrollCopier);
#endif
#ifdef __AVX2__
SHM_COPY_BENCHMARK(shm::memory::dragons::AvxAsyncCopier);
SHM_COPY_BENCHMARK(shm::memory::dragons::AvxAsyncPFCopier);
SHM_COPY_BENCHMARK(shm::memory::dragons::AvxAsyncUnrollCopier);
SHM_COPY_BENCHMARK(shm::memory::dragons::AvxAsyncPFUnrollCopier);
#endif

BENCHMARK_MAIN();
