//
// Created by squadrick on 8/2/19.
//

#include <shadesmar/custom_msg.h>
#include <shadesmar/pubsub/publisher.h>

int main(int argc, char **argv) {
  uint32_t timeout;

  if (argc == 1) {
    timeout = 1;
  } else {
    timeout = std::atoi(argv[1]);
  }

  shm::pubsub::Publisher<CustomMessage, 16> p("benchmark_topic");
  for (int i = 0; i < 1000000; ++i) {
    CustomMessage msg(1280 * 720 * 16);
    msg.fill(i % 256);
    msg.init_time();
    p.publish(msg);
    std::cout << "Publishing " << i << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
  }
}