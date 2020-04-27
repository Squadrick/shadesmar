//
// Created by squadrick on 19/01/20.
//

#include <shadesmar/rpc/server.h>

float fmadd(float a, float b, float c) { return a * b + c; }

int main() {
  shm::rpc::Function<float(float, float, float)> fmadd_rpc("fmadd", fmadd);

  while (true) {
    fmadd_rpc.serve_once();
  }
}