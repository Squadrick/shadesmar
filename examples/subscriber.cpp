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

#include "shadesmar/pubsub/subscriber.h"

#include "./custom_msg.h"

void callback(const std::shared_ptr<CustomMessage> &msg) {
  std::cout << msg->seq << "\t";
  auto lag = std::chrono::duration_cast<TIMESCALE>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count() -
             msg->timestamp;
  std::cout << "Avg lag: " << lag << TIMESCALE_NAME << std::endl;

  for (auto i : msg->arr) {
    if (i != msg->val) {
      std::cerr << "Pubsub error" << std::endl;
      return;
    }
  }
}

int main() {
  shm::pubsub::Subscriber<CustomMessage, 16> sub("example_topic", callback,
                                                 true);

  while (true) {
    sub.spin_once();
  }
}
