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

#ifndef EXAMPLES_CUSTOM_MSG_H_
#define EXAMPLES_CUSTOM_MSG_H_

#include <string>
#include <vector>

#include "shadesmar/message.h"

class InnerMessage : public shm::BaseMsg {
 public:
  int inner_val{};
  std::string inner_str{};
  SHM_PACK(inner_val, inner_str);

  InnerMessage() = default;
};

class CustomMessage : public shm::BaseMsg {
 public:
  uint8_t val{};
  std::vector<uint8_t> arr;
  InnerMessage im;
  SHM_PACK(val, arr, im);

  int size;

  explicit CustomMessage(int n) : size(n) {}

  void fill(uint8_t fill_val) {
    val = fill_val;
    for (int i = 0; i < size; ++i) arr.push_back(val);
  }

  // MUST BE INCLUDED
  CustomMessage() = default;
};

#endif  // EXAMPLES_CUSTOM_MSG_H_
