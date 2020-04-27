//
// Created by squadrick on 17/12/19.
//

#include <shadesmar/rpc/client.h>
#include <shadesmar/rpc/server.h>

int fn(int a, int b) { return a + b; }

void call_run() {
  shm::rpc::FunctionCaller rpc_fn("inc");
  for (int i = 0; true; ++i) {
    std::cout << rpc_fn(i, i).as<int>() << std::endl;
  }
}

int main() {
  if (fork() != 0) {
    shm::rpc::Function<int(int, int)> rpc_fn("inc", fn);
    while (true)
      rpc_fn.serve_once();
  } else {
    //    fork();
    call_run();
  }
}