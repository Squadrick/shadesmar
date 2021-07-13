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

#include <cstring>
#include <iostream>

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/dragons.h"
#endif

void mem_check(void *mem, size_t mem_size, int num) {
  for (size_t i = 0; i < mem_size; ++i) {
    char *ptr = reinterpret_cast<char *>(mem);
    if (ptr[i] != num) {
      std::cout << "Failed" << std::endl;
      exit(1);
    }
  }
}

template <class CopierT>
void run_test(size_t mem_size, int num) {
  mem_size += num;  // mis-aligns the data
  std::cout << "Test for: " << typeid(CopierT).name() << std::endl;
  CopierT cpy;

  void *shm = cpy.alloc(mem_size);
  void *user = cpy.alloc(mem_size);
  std::memset(shm, num, mem_size);

  cpy.shm_to_user(user, shm, mem_size);
  mem_check(user, mem_size, num);

  cpy.user_to_shm(shm, user, mem_size);
  mem_check(shm, mem_size, num);

  cpy.dealloc(shm);
  cpy.dealloc(user);
}

template <class CopierT>
void test(size_t mem_size, int num, bool mt = true) {
  run_test<CopierT>(mem_size, num);
  if (mt) {
    run_test<shm::memory::dragons::MTCopier<CopierT, 4>>(mem_size, num);
  }
}

int main() {
  for (uint32_t i = 4; false && i < 15; i += 3) {
    size_t mem_size = 1 << i;
    std::cout << "Memory size: 2 ^ " << i << std::endl;
    test<shm::memory::DefaultCopier>(mem_size, i);
#ifdef __x86_64__
    test<shm::memory::dragons::RepMovsbCopier>(mem_size, i);
#endif
#ifdef __AVX__
    test<shm::memory::dragons::AvxCopier>(mem_size, i);
    test<shm::memory::dragons::AvxUnrollCopier>(mem_size, i);
#endif
#ifdef __AVX2__
    test<shm::memory::dragons::AvxAsyncCopier>(mem_size, i);
    test<shm::memory::dragons::AvxAsyncPFCopier>(mem_size, i);
    test<shm::memory::dragons::AvxAsyncUnrollCopier>(mem_size, i);
    test<shm::memory::dragons::AvxAsyncPFUnrollCopier>(mem_size, i);
#endif
  }
}
