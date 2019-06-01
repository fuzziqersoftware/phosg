#include "FileCache.hh"

#include <unistd.h>

#include "Filesystem.hh"

using namespace std;

// windows doesn't have fork/exec, so O_CLOEXEC is meaningless
#ifdef WINDOWS
#define O_CLOEXEC 0
#endif


FileCache::File::File(const std::string& name, scoped_fd&& fd) : name(name),
    fd(move(fd)) { }


FileCache::FileCache(size_t max_size) : max_size(max_size) { }

size_t FileCache::get_max_size() const {
  return this->max_size;
}

size_t FileCache::set_max_size(size_t max_size) {
  lock_guard<mutex> g(this->lock);
  size_t ret = this->max_size;
  this->max_size = max_size;
  this->enforce_max_size();
  return ret;
}

bool FileCache::is_open(const string& filename) const {
  lock_guard<mutex> g(this->lock);
  return this->name_to_file.count(filename);
}

size_t FileCache::size() const {
  lock_guard<mutex> g(this->lock);
  return this->name_to_file.size();
}

shared_ptr<const FileCache::File> FileCache::lease(const string& name,
    mode_t create_mode) {
  try {
    lock_guard<mutex> g(this->lock);
    auto ret = this->name_to_file.at(name);
    this->lru.touch(ret->name);
    return ret;

  } catch (const out_of_range& e) {
    int fd = open(name.c_str(),
        O_RDWR | (create_mode ? O_CREAT : 0) | O_CLOEXEC, create_mode);
    if (fd < 0) {
      throw cannot_open_file(name);
    }

    shared_ptr<File> fd_ptr(new File(name, fd));
    shared_ptr<File> ret = fd_ptr;

    {
      lock_guard<mutex> g(this->lock);
      auto emplace_ret = this->name_to_file.emplace(name, fd_ptr);
      if (emplace_ret.second) {
        this->lru.insert(fd_ptr->name);
        this->enforce_max_size();
      } else {
        // if we get here, then there was a data race and another thread opened
        // this file while we were trying ot open it. we'll close our fd and use
        // the one they opened instead. (note that fd_ptr is destroyed outside
        // of the lock scope, so close() isn't called while holding the lock)
        ret = emplace_ret.first->second;
      }
    }
    return ret;
  }
}

void FileCache::close(const std::string& name) {
  lock_guard<mutex> g(this->lock);

  auto fd_it = this->name_to_file.find(name);
  if (fd_it == this->name_to_file.end()) {
    return;
  }

  this->lru.erase(fd_it->second->name);
  this->name_to_file.erase(fd_it);
}

void FileCache::clear() {
  lock_guard<mutex> g(this->lock);
  this->lru.clear();
  this->name_to_file.clear();
}

void FileCache::enforce_max_size() {
  while (this->lru.count() > this->max_size) {
    auto evicted_fd_name = this->lru.evict_object().first;
    this->name_to_file.erase(evicted_fd_name);
  }
}
