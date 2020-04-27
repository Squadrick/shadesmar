//
// Created by squadrick on 19/01/20.
//

#include <random>

#include <shadesmar/rpc/client.h>

int main() {
  shm::rpc::FunctionCaller fmadd_fn("fmadd");

  std::random_device device;
  std::mt19937 generator(device());

  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  float a = dist(generator);
  float b = dist(generator);
  float c = dist(generator);

  std::cout << fmadd_fn(a, b, c).as<float>() << std::endl;
}