//
// Created by squadrick on 14/11/19.
//

#ifndef SHADESMAR_ALLOCATOR_H
#define SHADESMAR_ALLOCATOR_H

namespace shm
{
class Allocator
{
public:
  Allocator(void *raw_buffer);
  void* malloc(size_t size);
  void free(void *ptr);
};
}
#endif // SHADESMAR_ALLOCATOR_H
