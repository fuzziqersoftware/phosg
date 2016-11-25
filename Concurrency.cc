#include "Concurrency.hh"

using namespace std;


rw_lock::rw_lock() {
  pthread_rwlock_init(&this->lock, NULL);
}

rw_lock::~rw_lock() {
  pthread_rwlock_destroy(&this->lock);
}

rw_guard::rw_guard(rw_lock& lock, bool exclusive) : lock(&lock.lock) {
  if (exclusive) {
    pthread_rwlock_wrlock(this->lock);
  } else {
    pthread_rwlock_rdlock(this->lock);
  }
}

rw_guard::rw_guard(pthread_rwlock_t* lock, bool exclusive) : lock(lock) {
  if (exclusive) {
    pthread_rwlock_wrlock(this->lock);
  } else {
    pthread_rwlock_rdlock(this->lock);
  }
}

rw_guard::rw_guard(rw_guard&& g) : lock(g.lock) {
  g.lock = NULL;
}

rw_guard::~rw_guard() {
  if (this->lock) {
    pthread_rwlock_unlock(this->lock);
  }
}


rw_guard_multi::rw_guard_multi() : guards() { }

rw_guard_multi::rw_guard_multi(size_t count) : guards() {
  this->guards.reserve(count);
}

rw_guard_multi::rw_guard_multi(rw_guard_multi&& g) : guards(move(g.guards)) { }

rw_guard_multi::~rw_guard_multi() {
  while (!this->guards.empty()) {
    this->guards.pop_back();
  }
}

void rw_guard_multi::add(rw_guard&& g) {
  this->guards.emplace_back(move(g));
}

void rw_guard_multi::add(rw_lock& lock, bool exclusive) {
  this->guards.emplace_back(lock, exclusive);
}

void rw_guard_multi::add(pthread_rwlock_t* lock, bool exclusive) {
  this->guards.emplace_back(lock, exclusive);
}

void rw_guard_multi::unlock_latest() {
  this->guards.pop_back();
}
