//
// Created by squadrick on 8/2/19.
//

#include <shadesmar/custom_msg.h>
#include <shadesmar/publisher.h>

int main(int argc, char **argv) {
  uint32_t timeout;

  if (argc == 1) {
    timeout = 1;
  } else {
    timeout = std::atoi(argv[1]);
  }

  shm::Publisher<CustomMessage, 16> p("test");
  CustomMessage msg(1280 * 720 * 16);
  msg.frame_id = "test123";
  for (int i = 0;; ++i) {
    msg.init_time(shm::SYSTEM);
    p.publish(msg);
    std::cout << "Publishing " << i << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
  }
}