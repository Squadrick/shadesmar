//
// Created by squadrick on 17/12/19.
//

#include <shadesmar/client.h>
#include <shadesmar/server.h>
#include <shadesmar/template_magic.h>

#include <any>

using namespace shm;
int fn(int a) { return a + 1; }

template <class... Params>
std::tuple<Params...> get_tuple(template_magic::types<Params...>) {
  std::tuple<Params...> arr;
  return arr;
}

int main() {
  shm::rpc::Client client;
  auto buf = client.call("dec", "tset", 3);

  shm::rpc::Function<int(int)> inc("inc", fn);
  std::cout << inc.serveOnce(buf) << std::endl;
}