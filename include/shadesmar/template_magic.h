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

#ifndef INCLUDE_SHADESMAR_TEMPLATE_MAGIC_H_
#define INCLUDE_SHADESMAR_TEMPLATE_MAGIC_H_
namespace shm::template_magic {
template <class F, class... Args>
void do_for(F fn, Args... args) {
  int _[] = {(fn(args), 0)...};
}

template <class...>
struct types {
  using type = types;
};
template <class Sig>
struct args;
template <class R, class... Args>
struct args<R(Args...)> : types<Args...> {};
template <class Sig>
using args_t = typename args<Sig>::type;

}  // namespace shm::template_magic
#endif  // INCLUDE_SHADESMAR_TEMPLATE_MAGIC_H_
