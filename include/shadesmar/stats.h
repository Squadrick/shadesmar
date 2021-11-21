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

#ifndef INCLUDE_SHADESMAR_STATS_H_
#define INCLUDE_SHADESMAR_STATS_H_

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace shm::stats {

class Welford {
 public:
  Welford() { clear(); }

  void clear() {
    n = 0;
    old_mean = new_mean = 0.0;
    old_dev = new_dev = 0.0;
  }

  void add(double value) {
    n++;
    if (n == 1) {
      old_mean = new_mean = value;
      old_dev = 0.0;
    } else {
      new_mean = old_mean + (value - old_mean) / n;
      new_dev = old_dev + (value - old_mean) * (value - new_mean);

      old_mean = new_mean;
      old_dev = new_dev;
    }
  }

  size_t size() const { return n; }

  double mean() const {
    if (n > 0) {
      return new_mean;
    }
    return 0.0;
  }

  double variance() const {
    if (n > 1) {
      return new_dev / (n - 1);
    }
    return 0.0;
  }

  double std_dev() const { return std::sqrt(variance()); }

  friend std::ostream& operator<<(std::ostream& o, const Welford& w);

 private:
  size_t n{};
  double old_mean{}, new_mean{}, old_dev{}, new_dev{};
};

std::ostream& operator<<(std::ostream& o, const Welford& w) {
  return o << w.mean() << " ± " << w.std_dev() << " (" << w.size() << ")";
}

}  // namespace shm::stats

#endif  // INCLUDE_SHADESMAR_STATS_H_
