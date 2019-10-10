//
// Created by squadrick on 8/2/19.
//

#include <shadesmar/custom_msg.h>
#include <shadesmar/publisher.h>

int main() {
  shm::Publisher<CustomMessage, 16> p("test");
  int a = 0;

  for (int i = 0;; ++i) {
    CustomMessage msg(i % 16);
    msg.frame_id = "test123";
    msg.init_time(shm::msg::SYSTEM);

    std::cout << "Publishing " << a << std::endl;
    p.publish(msg);
    ++a;
  }
}