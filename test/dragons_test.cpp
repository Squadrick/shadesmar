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

#include <cassert>
#include <cstring>

#include <iostream>

#include "shadesmar/macros.h"
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/dragons.h"
#include "shadesmar/memory/memory.h"

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
void test(size_t mem_size, int num) {
  CopierT cpy;
  bool segment;
  void *user;

  shm_unlink("dragons_test");
  void *shm =
      shm::memory::create_memory_segment("dragons_test", mem_size, &segment);

  std::cout << "Test for: " << typeid(CopierT).name() << std::endl;

  std::memset(shm, num, mem_size);

  TIMEIT(user = cpy.alloc(mem_size), "Alloc");

  TIMEIT(cpy.shm_to_user(user, shm, mem_size), "Shm->User");
  mem_check(user, mem_size, num);

  TIMEIT(cpy.user_to_shm(shm, user, mem_size), "User->Shm");
  mem_check(shm, mem_size, num);

  TIMEIT(cpy.dealloc(user), "Dealloc");
  shm_unlink("dragons_test");
  std::cout << std::endl;
}

int main() {
  for (int i = 11; i < 30; i += 3) {
    size_t mem_size = 1 << i;
    std::cout << "Memory size: 2 ^ " << i << std::endl;
    test<shm::memory::DefaultCopier>(mem_size, i);
#ifndef __APPLE__  // no MaxOS support
    test<shm::memory::dragons::RepMovsbCopier>(mem_size, i + 1);
#endif
  }
}
