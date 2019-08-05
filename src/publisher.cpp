//
// Created by squadrick on 8/2/19.
//

#include <shadesmar/message.h>
#include <shadesmar/publisher.h>
#include <unistd.h>

int main() {
  shm::Publisher<shm::Msg> p("test1", 10);
  int a = 0, loops = 1000;
  auto *msg = new shm::Msg;

  for (int i = 0; i < loops; ++i) {
    std::memset(msg->bytes, a % 256, MSGSIZE);
    msg->count = a % 256;
    std::cout << "Publishing " << a << std::endl;
    p.publish(msg);
    ++a;
  }
  delete msg;
}