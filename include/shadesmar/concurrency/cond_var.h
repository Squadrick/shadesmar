//
// Created by squadrick on 26/04/20.
//

#ifndef SHADESMAR_COND_VAR_H
#define SHADESMAR_COND_VAR_H

#include <pthread.h>

#include <shadesmar/concurrency/lock.h>

namespace shm {

class CondVar {
public:
  CondVar();
  ~CondVar();

  void wait(PthreadWriteLock &lock);
  void signal();

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

void CondVar::wait(PthreadWriteLock &lock) {
  pthread_cond_wait(&cond, lock.get_mutex());
}

void CondVar::signal() { pthread_cond_signal(&cond); }

} // namespace shm

#endif // SHADESMAR_COND_VAR_H
