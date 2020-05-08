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

#include <shadesmar/macros.h>
#include <shadesmar/memory/memory.h>
#include <shadesmar/memory/tmp.h>

int main() {
  bool creat;
  auto *ptr =
      shm::memory::create_memory_segment("test_shm", sizeof(int) * 100, creat);

  if (creat) {
    shm::memory::tmp::write("test_shm");
    std::cout << "Writing" << std::endl;
    for (int i = 0; i < 100; ++i) {
      ptr[i] = i;
    }

    while (true) {
    }
  } else {
    std::cout << "Reading" << std::endl;
    for (int i = 0; i < 100; ++i) std::cout << ptr[i] << std::endl;
  }
}
