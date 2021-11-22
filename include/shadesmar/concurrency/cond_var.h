/* MIT License

Copyright (c) 2020 Dheeraj R Reddy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

#ifndef INCLUDE_SHADESMAR_CONCURRENCY_COND_VAR_H_
#define INCLUDE_SHADESMAR_CONCURRENCY_COND_VAR_H_

#include <pthread.h>

#include "shadesmar/concurrency/lock.h"

namespace shm::concurrent {

class CondVar {
 public:
  CondVar();
  ~CondVar();

  void wait(PthreadWriteLock *lock);
  void signal();
  void reset();

 private:
  pthread_condattr_t attr;
  pthread_cond_t cond;
};

CondVar::CondVar() {
  pthread_condattr_init(&attr);
  pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&cond, &attr);
}

CondVar::~CondVar() {
  pthread_condattr_destroy(&attr);
  pthread_cond_destroy(&cond);
}

void CondVar::wait(PthreadWriteLock *lock) {
  pthread_cond_wait(&cond, lock->get_mutex());
}

void CondVar::signal() { pthread_cond_signal(&cond); }

void CondVar::reset() {
  pthread_cond_destroy(&cond);
  pthread_cond_init(&cond, &attr);
}

}  // namespace shm::concurrent

#endif  // INCLUDE_SHADESMAR_CONCURRENCY_COND_VAR_H_
