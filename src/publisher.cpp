//
// Created by squadrick on 8/2/19.
//

#include <shadesmar/publisher.h>

int main() {
  shm::Publisher<int> p("test", 10);

  int a = 0;

  while (true) {
    p.publish(a);
    ++a;
  }
}