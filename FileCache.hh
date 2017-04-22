#pragma once

#include <mutex>
#include <string>

#include "Filesystem.hh"
#include "LRUSet.hh"

using namespace std;


class FileCache {
private:
  struct File {
    std::string name;
    scoped_fd fd;

    File() = delete;
    File(const std::string& name, scoped_fd&& fd);
    File(const FileCache&) = delete;
    File(FileCache&&) = delete;
    File& operator=(const FileCache&) = delete;
    ~File() = default;
  };

public:
  FileCache() = delete;
  explicit FileCache(size_t max_size);
  FileCache(const FileCache&) = delete;
  FileCache(FileCache&&) = delete;
  FileCache& operator=(const FileCache&) = delete;
  ~FileCache() = default;

  size_t get_max_size() const;
  size_t set_max_size(size_t max_size);

  bool is_open(const std::string& name) const;
  size_t size() const;

  struct Lease {
    int fd;
    std::shared_ptr<File> fd_object;

    Lease() = delete;
    Lease(FileCache* cache, const std::string& name, mode_t create_mode);
    Lease(const Lease&) = delete;
    Lease(Lease&&) = default;
    Lease& operator=(const Lease&) = delete;
    ~Lease() = default;

    void close();
  };
  Lease lease(const std::string& name, mode_t create_mode = 0644);

  void close(const std::string& name);

  void clear();

private:
  size_t max_size;
  LRUSet<std::shared_ptr<File>> lru;
  std::unordered_map<std::string, std::shared_ptr<File>> name_to_fd;
  mutable std::mutex lock;

  void enforce_max_size();
};
