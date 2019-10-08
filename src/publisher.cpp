//
// Created by squadrick on 8/2/19.
//

#include <shadesmar/message.h>
#include <shadesmar/publisher.h>

int main() {
  shm::Publisher<shm::Msg<1024>, 16> p("test");
  int a = 0, loops = 1000;
  auto *msg = new shm::Msg<1024>;

  for (int i = 0;; ++i) {
    msg->setVal(a & 255);
    std::cout << "Publishing " << a << std::endl;
    p.publish(msg);
    ++a;
  }
  delete msg;
}