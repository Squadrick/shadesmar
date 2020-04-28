//
// Created by squadrick on 17/12/19.
//

#include <thread>

#include <catch.hpp>

#include <shadesmar/rpc/client.h>
#include <shadesmar/rpc/server.h>

TEST_CASE("RPC client server") {
  uint32_t iter = 10;
  shm::rpc::Function<int(int, int)> serve_fn(
      "inc", [](int a, int b) { return a + b; });
  shm::rpc::FunctionCaller call_fn("inc");

  std::thread server([&]() {
    for (uint32_t i = 0; i < 10000; ++i) {
      serve_fn.serve_once();
    }
  });
  REQUIRE(call_fn(10, 10).as<int>() == 20);
  server.join();
}