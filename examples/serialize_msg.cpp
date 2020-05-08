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

#include <iostream>
#include <msgpack.hpp>

#include "./custom_msg.h"

int main() {
  CustomMessage cm(5);
  cm.seq = 1;
  cm.init_time();
  cm.im.inner_val = 100;
  cm.im.inner_str = "Dheeraj";

  std::cout << cm.timestamp << std::endl;

  msgpack::sbuffer buf;
  msgpack::pack(buf, cm);

  std::cout << buf.data() << std::endl;

  msgpack::object_handle res = msgpack::unpack(buf.data(), buf.size());
  msgpack::object obj = res.get();
  std::cout << obj << std::endl;

  CustomMessage mc;
  obj.convert(mc);

  std::cout << mc.seq << std::endl;
  std::cout << mc.timestamp << std::endl;
  std::cout << mc.im.inner_val << std::endl;
  std::cout << mc.im.inner_str << std::endl;

  for (auto &x : mc.arr) std::cout << x << " ";
  std::cout << std::endl;
}
