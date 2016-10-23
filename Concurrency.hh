#pragma once

#include <pthread.h>

class rw_lock {
private:
  pthread_rwlock_t lock;

public:
  rw_lock();
  ~rw_lock();

  friend class rw_guard;
};

class rw_guard {
private:
  pthread_rwlock_t* lock;

public:
  rw_guard() = delete;
  rw_guard(rw_lock& lock, bool exclusive);
  ~rw_guard();
};
