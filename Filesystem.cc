#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;


unordered_set<string> list_directory(const string& dirname,
    std::function<bool(struct dirent*)> filter) {

  DIR* dir = opendir(dirname.c_str());
  if (dir == NULL) {
    throw runtime_error("Can\'t read directory " + dirname);
  }

  struct dirent* entry;
  unordered_set<string> files;
  while ((entry = readdir(dir))) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
        (filter && !filter(entry))) {
      continue;
    }
    files.emplace(entry->d_name);
  }

  closedir(dir);
  return files;
}

cannot_stat_file::cannot_stat_file(int fd) :
    runtime_error("can\'t stat fd " + to_string(fd)) { }

cannot_stat_file::cannot_stat_file(const string& filename) :
    runtime_error("can\'t stat file " + filename) {}

cannot_open_file::cannot_open_file(const string& filename) :
    runtime_error("can\'t open file " + filename) {}

struct stat stat(const string& filename) {
  struct stat st;
  if (stat(filename.c_str(), &st)) {
    throw cannot_stat_file(filename);
  }
  return st;
}

struct stat lstat(const string& filename) {
  struct stat st;
  if (lstat(filename.c_str(), &st)) {
    throw cannot_stat_file(filename);
  }
  return st;
}

struct stat fstat(int fd) {
  struct stat st;
  if (fstat(fd, &st)) {
    throw cannot_stat_file(fd);
  }
  return st;
}

struct stat fstat(FILE* f) {
  return fstat(fileno(f));
}

string load_file(const string& filename) {
  auto f = fopen_unique(filename.c_str(), "rb");
  size_t file_size = fstat(fileno(f.get())).st_size;

  string data(file_size, 0);
  if (fread(const_cast<char*>(data.data()), 1, data.size(), f.get()) != data.size()) {
    throw runtime_error("can\'t read from " + filename + ": " + string_for_error(errno));
  }

  return data;
}

void save_file(const string& filename, const string& data) {
  auto f = fopen_unique(filename.c_str(), "wb");
  if (fwrite(data.data(), 1, data.size(), f.get()) != data.size()) {
    throw runtime_error("can\'t write to " + filename + ": " + string_for_error(errno));
  }
}

unique_ptr<FILE, void(*)(FILE*)> fopen_unique(const string& filename,
    const string& mode) {
  unique_ptr<FILE, void(*)(FILE*)> f(
    fopen(filename.c_str(), mode.c_str()),
    [](FILE* f) {
      fclose(f);
    });
  if (!f.get()) {
    throw cannot_open_file(filename);
  }
  return f;
}
