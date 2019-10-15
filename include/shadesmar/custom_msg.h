//
// Created by squadrick on 10/10/19.
//

#include <shadesmar/message.h>
#include <iostream>

class InnerMessage : public shm::msg::BaseMsg {
 public:
  int inner_val{};
  std::string inner_str{};
  SHM_PACK(inner_val, inner_str);

  InnerMessage() = default;
};

class CustomMessage : public shm::msg::BaseMsg {
 public:
  int val{};
  std::vector<uint8_t> arr;
  InnerMessage im;
  SHM_PACK(val, arr, im);

  explicit CustomMessage(int n) {
    val = n;
    for (int i = 0; i < n; ++i) {
      arr.push_back(val);
    }
  }

  // MUST BE INCLUDED
  CustomMessage() = default;
};
