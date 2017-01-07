#include "FileCache.hh"

#include <unistd.h>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;


FileCache::File::File(const std::string& name, scoped_fd&& fd) : name(name),
    fd(move(fd)) { }


FileCache::FileCache(size_t max_size) : max_size(max_size), lru(), name_to_fd(),
    lock() { }

size_t FileCache::get_max_size() const {
  return this->max_size;
}

size_t FileCache::set_max_size(size_t max_size) {
  size_t ret = this->max_size;
  this->max_size = max_size;
  this->enforce_max_size();
  return ret;
}

size_t FileCache::size() const {
  return this->name_to_fd.size();
}

FileCache::Lease::Lease(FileCache* cache, const std::string& name,
    mode_t create_mode) : cache(cache), fd(-1) {

  try {
    lock_guard<mutex> g(cache->lock);
    this->fd_object = cache->name_to_fd.at(name);
    cache->lru.touch(this->fd_object);

  } catch (const out_of_range& e) {
    this->fd = open(name.c_str(),
        O_RDWR | (create_mode ? O_CREAT : 0) | O_CLOEXEC, create_mode);
    if (this->fd < 0) {
      throw cannot_open_file(name);
    }

    shared_ptr<File> fd_ptr(new File(name, this->fd));

    {
      lock_guard<mutex> g(cache->lock);
      auto emplace_ret = cache->name_to_fd.emplace(name, fd_ptr);
      this->fd_object = emplace_ret.first->second;

      if (emplace_ret.second) {
        this->fd = this->fd_object->fd;

        cache->lru.insert(fd_ptr);
        cache->enforce_max_size();
        return;
      }
    }

    // another thread got there first and opened this fd; close this fd and use
    // the other one instead
    ::close(this->fd);
  }

  this->fd = this->fd_object->fd;
}

FileCache::Lease FileCache::lease(const std::string& name, mode_t create_mode) {
  return Lease(this, name, create_mode);
}

void FileCache::close(const std::string& name) {
  lock_guard<mutex> g(this->lock);

  auto fd_it = this->name_to_fd.find(name);
  if (fd_it == this->name_to_fd.end()) {
    return;
  }

  this->lru.erase(fd_it->second);
  this->name_to_fd.erase(fd_it);
}

void FileCache::enforce_max_size() {
  while (this->lru.size() > this->max_size) {
    auto evicted_fd_object = this->lru.evict_object().first;
    this->name_to_fd.erase(evicted_fd_object->name);
  }
}
