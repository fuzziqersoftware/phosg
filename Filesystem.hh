#pragma once

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


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
  explicit scoped_fd(int fd);
  scoped_fd(const char* filename, int mode, mode_t perm = 0755);
  scoped_fd(const std::string& filename, int mode, mode_t perm = 0755);
  scoped_fd(const scoped_fd&) = delete;
  scoped_fd(scoped_fd&&);
  ~scoped_fd();
  scoped_fd& operator=(scoped_fd& other) = delete;
  scoped_fd& operator=(int other);

  operator int() const;

  void open(const char* filename, int mode, mode_t perm = 0755);
  void open(const std::string& filename, int mode, mode_t perm = 0755);
  void close();

  bool is_open();

private:
  int fd;
};

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

std::string load_file(const std::string& filename);
void save_file(const std::string& filename, const std::string& data);

std::unique_ptr<FILE, void(*)(FILE*)> fopen_unique(const std::string& filename,
    const std::string& mode = "rb");
std::unique_ptr<FILE, void(*)(FILE*)> fdopen_unique(int fd,
    const std::string& mode = "rb");
std::shared_ptr<FILE> fopen_shared(const std::string& filename,
    const std::string& mode = "rb");
std::shared_ptr<FILE> fdopen_shared(int fd, const std::string& mode = "rb");

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
