#pragma once

#include <pthread.h>

class rw_lock { // goddamnit why isn't shared_mutex in c++14
private:
  pthread_rwlock_t lock;

public:
  explicit rw_lock();
  ~rw_lock();

  friend class rw_guard;
};

class rw_guard {
private:
  pthread_rwlock_t* lock;

public:
  rw_guard() = delete;
  explicit rw_guard(rw_lock& lock, bool exclusive);
  ~rw_guard();
};
