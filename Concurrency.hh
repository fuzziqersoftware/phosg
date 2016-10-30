#pragma once

#include <pthread.h>
#include <stdint.h>

#include <vector>

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
  rw_guard(const rw_guard& g) = delete;
  rw_guard(rw_guard&& g);
  rw_guard(rw_lock& lock, bool exclusive);
  ~rw_guard();

  rw_guard& operator=(const rw_guard& g) = delete;
};

class rw_guard_multi {
private:
  std::vector<rw_guard> guards;

public:
  rw_guard_multi();
  explicit rw_guard_multi(size_t count);
  rw_guard_multi(const rw_guard& g) = delete;
  rw_guard_multi(rw_guard_multi&& g);
  ~rw_guard_multi();

  rw_guard& operator=(const rw_guard& g) = delete;

  void add(rw_guard&& g);
  void add(rw_lock& lock, bool exclusive);
  void unlock_latest();
};
