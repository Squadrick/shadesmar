//
// Created by squadrick on 10/10/19.
//

#include <shadesmar/custom_msg.h>
#include <msgpack.hpp>

int main() {
  CustomMessage cm(5);
  cm.seq = 1;
  cm.frame_id = "test123";
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
  std::cout << mc.frame_id << std::endl;
  std::cout << mc.timestamp << std::endl;
  std::cout << mc.im.inner_val << std::endl;
  std::cout << mc.im.inner_str << std::endl;

  for (auto &x : mc.arr) std::cout << x << " ";
  std::cout << std::endl;
}