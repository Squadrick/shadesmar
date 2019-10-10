//
// Created by squadrick on 06/10/19.
//

#ifndef shadesmar_MACROS_H
#define shadesmar_MACROS_H

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

#define INFO_INIT 1337

#endif  // shadesmar_MACROS_H
