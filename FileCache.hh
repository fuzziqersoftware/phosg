#pragma once

#include <mutex>
#include <string>

#include "Filesystem.hh"
#include "LRUSet.hh"

using namespace std;


class FileCache {
public:
  struct File {
    std::string name;
    scoped_fd fd;

    File() = delete;
    File(const std::string& name, scoped_fd&& fd);
    File(const File&) = delete;
    File(File&&) = delete;
    File& operator=(const File&) = delete;
    File& operator=(File&&) = delete;
    ~File() = default;
  };

  FileCache() = delete;
  explicit FileCache(size_t max_size);
  FileCache(const FileCache&) = delete;
  FileCache(FileCache&&) = delete;
  FileCache& operator=(const FileCache&) = delete;
  FileCache& operator=(FileCache&&) = delete;
  ~FileCache() = default;

  size_t get_max_size() const;
  size_t set_max_size(size_t max_size);

  bool is_open(const std::string& name) const;
  size_t size() const;

  shared_ptr<const File> lease(const std::string& name, mode_t create_mode = 0644);

  void close(const std::string& name);

  void clear();

private:
  size_t max_size;
  LRUSet<std::string> lru;
  std::unordered_map<std::string, std::shared_ptr<File>> name_to_file;
  mutable std::mutex lock;

  void enforce_max_size();
};
