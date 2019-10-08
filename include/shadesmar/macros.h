//
// Created by squadrick on 06/10/19.
//

#ifndef SHADERMAR_MACROS_H
#define SHADERMAR_MACROS_H

#ifdef DEBUG_BUILD
#define DEBUG(str)                 \
  do {                             \
    std::cout << str << std::endl; \
  } while (0)
#else
#define DEBUG(str) \
  do {             \
  } while (0)
#endif

#define TIMEOUT 20

#define WAIT_CONTINUE(timeout) \
  usleep(timeout);             \
  continue;

#define INFO_INIT 1337

#endif  // SHADERMAR_MACROS_H
