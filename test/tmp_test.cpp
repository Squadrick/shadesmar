//
// Created by squadrick on 10/09/19.
//

#include <iostream>
#include <vector>

#include <catch.hpp>

#include <shadesmar/memory/tmp.h>

TEST_CASE("shm::memory::tmp write to /tmp") {
  shm::memory::tmp::delete_topics();

  SECTION("Write 10 names") {
    std::vector<std::string> mem_names(10);
    for (auto &mem_name : mem_names) {
      mem_name = shm::memory::tmp::random_string(16);
      shm::memory::tmp::write(mem_name);
    }
    auto tmp_names = shm::memory::tmp::get_tmp_names();

    std::sort(mem_names.begin(), mem_names.end());
    std::sort(tmp_names.begin(), tmp_names.end());

    REQUIRE(mem_names == tmp_names);
  }
  shm::memory::tmp::delete_topics();
}