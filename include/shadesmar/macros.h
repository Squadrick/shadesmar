//
// Created by squadrick on 06/10/19.
//

#ifndef shadesmar_MACROS_H
#define shadesmar_MACROS_H

#include <boost/timer/timer.hpp>

#define TIMESCALE std::chrono::microseconds
#define TIMESCALE_COUNT 1e6
#define TIMESCALE_NAME "microseconds"

#ifdef DEBUG_BUILD
#define DEBUG(str)                                                             \
  do {                                                                         \
    std::cout << str << std::endl;                                             \
  } while (0)

#define TIMEIT(cmd, name)                                                      \
  do {                                                                         \
    auto start = std::chrono::system_clock::now();                             \
    cmd;                                                                       \
    auto end = std::chrono::system_clock::now();                               \
    auto diff = std::chrono::duration_cast<TIMESCALE>(end - start).count();    \
    std::cout << "Time for " << name << ": " << diff << std::endl;             \
  } while (0);

#else

#define DEBUG(str)                                                             \
  do {                                                                         \
  } while (0)

#define TIMEIT(cmd, name) cmd;
#endif

#define INFO_INIT 1337

#endif // shadesmar_MACROS_H
