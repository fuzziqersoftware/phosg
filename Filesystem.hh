#pragma once

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Strings.hh"


std::unordered_set<std::string> list_directory(const std::string& dirname,
    std::function<bool(struct dirent*)> filter = NULL);

std::string get_user_home_directory();

class cannot_stat_file : virtual public std::runtime_error {
public:
  cannot_stat_file(int fd);
  cannot_stat_file(const std::string& filename);

  int error;
};

class cannot_open_file : virtual public std::runtime_error {
public:
  cannot_open_file(int fd);
  cannot_open_file(const std::string& filename);

  int error;
};

class io_error : virtual public std::runtime_error {
public:
  io_error(int fd);

  int error;
};

struct stat stat(const std::string& filename);
struct stat lstat(const std::string& filename);
struct stat fstat(int fd);
struct stat fstat(FILE* f);

bool isfile(const struct stat& st);
bool isdir(const struct stat& st);
bool lisfile(const struct stat& st);
bool lisdir(const struct stat& st);
bool islink(const struct stat& st);

bool isfile(const std::string& filename);
bool isdir(const std::string& filename);
bool lisfile(const std::string& filename);
bool lisdir(const std::string& filename);
bool islink(const std::string& filename);

std::string readlink(const std::string& filename);
std::string realpath(const std::string& path);

class scoped_fd {
public:
  scoped_fd();
  scoped_fd(int fd);
  scoped_fd(const char* filename, int mode, mode_t perm = 0755);
  scoped_fd(const std::string& filename, int mode, mode_t perm = 0755);
  scoped_fd(const scoped_fd&) = delete;
  scoped_fd(scoped_fd&&);
  ~scoped_fd();
  scoped_fd& operator=(const scoped_fd& other) = delete;
  scoped_fd& operator=(scoped_fd&& other);
  scoped_fd& operator=(int other);

  operator int() const;

  void open(const char* filename, int mode, mode_t perm = 0755);
  void open(const std::string& filename, int mode, mode_t perm = 0755);
  void close();

  bool is_open();

private:
  int fd;
};

std::string read_all(int fd);
std::string read_all(FILE* f);

void readx(int fd, void* data, size_t size);
void writex(int fd, const void* data, size_t size);
std::string readx(int fd, size_t size);
void writex(int fd, const std::string& data);

void preadx(int fd, void* data, size_t size, off_t offset);
void pwritex(int fd, const void* data, size_t size, off_t offset);
std::string preadx(int fd, size_t size, off_t offset);
void pwritex(int fd, const std::string& data, off_t offset);

void freadx(FILE* f, void* data, size_t size);
void fwritex(FILE* f, const void* data, size_t size);
std::string freadx(FILE* f, size_t size);
void fwritex(FILE* f, const std::string& data);

template <typename T>
T load_object_file(const std::string& filename) {
  scoped_fd fd(filename, O_RDONLY);
  if (fstat(fd).st_size != sizeof(T)) {
    throw std::runtime_error("size of " + filename + " is incorrect");
  }

  T ret;
  if (read(fd, &ret, sizeof(ret)) != sizeof(ret)) {
    throw std::runtime_error("can\'t read from " + filename + ": " + string_for_error(errno));
  }

  return ret;
}

template <typename T>
std::vector<T> load_vector_file(const std::string& filename) {
  scoped_fd fd(filename, O_RDONLY);
  size_t file_size = fstat(fd).st_size;
  size_t item_count = file_size / sizeof(T);
  size_t read_size = item_count * sizeof(T);

  std::vector<T> ret(item_count);
  if (read(fd, ret.data(), read_size) != read_size) {
    throw std::runtime_error("can\'t read from " + filename + ": " + string_for_error(errno));
  }

  return ret;
}

std::string load_file(const std::string& filename);
void save_file(const std::string& filename, const void* data, size_t size);
void save_file(const std::string& filename, const std::string& data);

std::unique_ptr<FILE, void(*)(FILE*)> fopen_unique(const std::string& filename,
    const std::string& mode = "rb", FILE* dash_file = NULL);
std::unique_ptr<FILE, void(*)(FILE*)> fdopen_unique(int fd,
    const std::string& mode = "rb");
std::shared_ptr<FILE> fopen_shared(const std::string& filename,
    const std::string& mode = "rb", FILE* dash_file = NULL);
std::shared_ptr<FILE> fdopen_shared(int fd, const std::string& mode = "rb");

void rename(const std::string& old_filename, const std::string& new_filename);
void unlink(const std::string& filename, bool recursive = false);

std::pair<int, int> pipe();

void make_fd_nonblocking(int fd);

class Poll {
public:
  Poll() = default;
  ~Poll() = default;

  void add(int fd, short events);
  void remove(int fd, bool close_fd = false);

  std::unordered_map<int, short> poll(int timeout_ms = 0);

private:
  std::vector<struct pollfd> poll_fds;
};
