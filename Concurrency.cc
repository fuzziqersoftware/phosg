#include "Concurrency.hh"

using namespace std;


rw_lock::rw_lock() {
#ifdef LINUX
  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init(&attr);
  pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
  pthread_rwlock_init(&this->lock, &attr);
#else
  pthread_rwlock_init(&this->lock, NULL);
#endif
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
