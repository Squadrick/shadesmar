//
// Created by squadrick on 06/10/19.
//

#ifndef shadesmar_MACROS_H
#define shadesmar_MACROS_H

#define TIMESCALE std::chrono::microseconds
#define TIMESCALE_COUNT 1e6
#define TIMESCALE_NAME "us"

#ifdef DEBUG_BUILD
#define DEBUG_IMPL(str, eol)                                                   \
  do {                                                                         \
    std::cout << str << eol;                                                   \
  } while (0);

#define TIMEIT(cmd, name)                                                      \
  do {                                                                         \
    auto start = std::chrono::system_clock::now();                             \
    cmd;                                                                       \
    auto end = std::chrono::system_clock::now();                               \
    auto diff = std::chrono::duration_cast<TIMESCALE>(end - start).count();    \
    DEBUG("Time for " << name << ": " << diff << TIMESCALE_NAME);              \
  } while (0);

#else

#define DEBUG_IMPL(str, eol)                                                   \
  do {                                                                         \
  } while (0);

#define TIMEIT(cmd, name) cmd;
#endif

#define DEBUG(str) DEBUG_IMPL(str, "\n");
#define INFO_INIT 1337

const int MAX_SHARED_OWNERS = 16;

#endif // shadesmar_MACROS_H
