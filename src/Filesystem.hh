#pragma once

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Platform.hh"

#include <dirent.h>
#include <poll.h>
#include <sys/uio.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

std::string basename(const std::string& filename);
std::string dirname(const std::string& filename);

std::unordered_set<std::string> list_directory(const std::string& dirname);
std::vector<std::string> list_directory_sorted(const std::string& dirname);

std::string getcwd();

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
  io_error(int fd, const std::string& what);

  int error;
};

struct stat stat(const std::string& filename);
struct stat lstat(const std::string& filename);
struct stat fstat(int fd);
struct stat fstat(FILE* f);

bool isfile(const struct stat& st);
bool isdir(const struct stat& st);
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

namespace std {
template <>
struct hash<scoped_fd> {
  size_t operator()(const scoped_fd& fd) const {
    return std::hash<int>()(static_cast<int>(fd));
  }
};
} // namespace std

std::string read_all(int fd);
std::string read_all(FILE* f);

std::string read(int fd, size_t size);
std::string fread(FILE* f, size_t size);

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
uint8_t fgetcx(FILE* f);

std::string fgets(FILE* f);

template <typename T>
T readx(int fd) {
  T t;
  readx(fd, &t, sizeof(T));
  return t;
}

template <typename T>
void writex(int fd, const T& t) {
  writex(fd, &t, sizeof(T));
}

template <typename T>
T preadx(int fd, off_t offset) {
  T t;
  preadx(fd, &t, sizeof(T), offset);
  return t;
}

template <typename T>
void pwritex(int fd, const T& t, off_t offset) {
  pwritex(fd, &t, sizeof(T), offset);
}

template <typename T>
T freadx(FILE* f) {
  T t;
  freadx(f, &t, sizeof(T));
  return t;
}

template <typename T>
void fwritex(FILE* f, const T& t) {
  fwritex(f, &t, sizeof(T));
}

template <typename T>
T load_object_file(const std::string& filename, bool allow_oversize = false) {
  scoped_fd fd(filename, O_RDONLY);
  T ret;
  readx(fd, &ret, sizeof(ret));
  if (!allow_oversize) {
    std::string extra = read(fd, 1);
    if (!extra.empty()) {
      throw std::runtime_error("file " + filename + " is too large");
    }
  }
  return ret;
}

template <typename T>
void save_object_file(const std::string& filename, const T& obj) {
  save_file(filename, &obj, sizeof(obj));
}

template <typename T>
std::vector<T> load_vector_file(const std::string& filename) {
  scoped_fd fd(filename, O_RDONLY);
  size_t file_size = fstat(fd).st_size;
  size_t item_count = file_size / sizeof(T);
  size_t read_size = item_count * sizeof(T);

  std::vector<T> ret(item_count);
  readx(fd, ret.data(), read_size);
  return ret;
}

template <typename T>
void save_vector_file(const std::string& filename, const std::vector<T>& v) {
  save_file(filename, v.data(), v.size() * sizeof(T));
}

std::string load_file(const std::string& filename);
void save_file(const std::string& filename, const void* data, size_t size);
void save_file(const std::string& filename, const std::string& data);

std::unique_ptr<FILE, void (*)(FILE*)> fopen_unique(const std::string& filename,
    const std::string& mode = "rb", FILE* dash_file = nullptr);
std::unique_ptr<FILE, void (*)(FILE*)> fdopen_unique(int fd,
    const std::string& mode = "rb");
std::unique_ptr<FILE, void (*)(FILE*)> fmemopen_unique(const void* buf,
    size_t size);
std::shared_ptr<FILE> fopen_shared(const std::string& filename,
    const std::string& mode = "rb", FILE* dash_file = nullptr);
std::shared_ptr<FILE> fdopen_shared(int fd, const std::string& mode = "rb");

void rename(const std::string& old_filename, const std::string& new_filename);
void unlink(const std::string& filename, bool recursive = false);

void make_fd_nonblocking(int fd);

std::pair<int, int> pipe();

class Poll {
public:
  Poll() = default;
  ~Poll() = default;

  void add(int fd, short events);
  void remove(int fd, bool close_fd = false);
  bool empty() const;

  std::unordered_map<int, short> poll(int timeout_ms = 0);

private:
  std::vector<struct pollfd> poll_fds;
};
