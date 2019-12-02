//
// Created by squadrick on 10/10/19.
//

#include <shadesmar/message.h>

class InnerMessage : public shm::BaseMsg {
public:
  int inner_val{};
  std::string inner_str{};
  SHM_PACK(inner_val, inner_str);

  InnerMessage() = default;
};

class CustomMessage : public shm::BaseMsg {
public:
  uint8_t val{};
  std::vector<uint8_t> arr;
  InnerMessage im;
  SHM_PACK(val, arr, im);

  explicit CustomMessage(int n) : arr(std::vector<uint8_t>(n)) {}

  void fill(uint8_t fill_val) {
    val = fill_val;
    std::fill(arr.begin(), arr.end(), fill_val);
  }

  // MUST BE INCLUDED
  CustomMessage() = default;
};
