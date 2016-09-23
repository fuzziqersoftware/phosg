#include "Concurrency.hh"


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

rw_guard::~rw_guard() {
  pthread_rwlock_unlock(this->lock);
}
