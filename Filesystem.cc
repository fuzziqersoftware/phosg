#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;


unordered_set<string> list_directory(const string& dirname,
    function<bool(struct dirent*)> filter) {

  DIR* dir = opendir(dirname.c_str());
  if (dir == NULL) {
    throw runtime_error("can\'t read directory " + dirname);
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

string get_user_home_directory() {
  const char *homedir = getenv("HOME");
  if (homedir) {
    return homedir;
  }

  int bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1) {
    throw runtime_error("can\'t get _SC_GETPW_R_SIZE_MAX");
  }

  char buffer[bufsize];
  struct passwd pwd, *result = NULL;
  if (getpwuid_r(getuid(), &pwd, buffer, bufsize, &result) != 0 || !result) {
    throw runtime_error("can\'t get home directory for current user");
  }

  return pwd.pw_dir;
}

cannot_stat_file::cannot_stat_file(int fd) :
    runtime_error("can\'t stat fd " + to_string(fd) + ": " + string_for_error(errno)),
    error(errno) { }

cannot_stat_file::cannot_stat_file(const string& filename) :
    runtime_error("can\'t stat file " + filename + ": " + string_for_error(errno)),
    error(errno) {}

cannot_open_file::cannot_open_file(const string& filename) :
    runtime_error("can\'t open file " + filename + ": " + string_for_error(errno)),
    error(errno) {}

io_error::io_error(int fd) :
    runtime_error("io error on fd " + to_string(fd) + ": " + string_for_error(errno)),
    error(errno) {}

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

bool isfile(const struct stat& st) {
  return (st.st_mode & S_IFMT) == S_IFREG;
}

bool isdir(const struct stat& st) {
  return (st.st_mode & S_IFMT) == S_IFDIR;
}

bool lisfile(const struct stat& st) {
  return (st.st_mode & S_IFMT) == S_IFREG;
}

bool lisdir(const struct stat& st) {
  return (st.st_mode & S_IFMT) == S_IFREG;
}

bool islink(const struct stat& st) {
  return (st.st_mode & S_IFMT) == S_IFLNK;
}

bool isfile(const string& filename) {
  try {
    return isfile(stat(filename));
  } catch (const cannot_stat_file& e) {
    return false;
  }
}

bool isdir(const string& filename) {
  try {
    return isdir(stat(filename));
  } catch (const cannot_stat_file& e) {
    return false;
  }
}

bool lisfile(const string& filename) {
  try {
    return isfile(lstat(filename));
  } catch (const cannot_stat_file& e) {
    return false;
  }
}

bool lisdir(const string& filename) {
  try {
    return isdir(lstat(filename));
  } catch (const cannot_stat_file& e) {
    return false;
  }
}

bool islink(const string& filename) {
  try {
    return islink(lstat(filename));
  } catch (const cannot_stat_file& e) {
    return false;
  }
}

string readlink(const string& filename) {
  string data(1024, 0);
  ssize_t length = readlink(filename.c_str(), const_cast<char*>(data.data()),
      data.size());
  if (length < 0) {
    throw cannot_stat_file(filename);
  }
  data.resize(length);
  data.shrink_to_fit();
  return data;
}

scoped_fd::scoped_fd() : fd(-1) { }

scoped_fd::scoped_fd(int fd) : fd(fd) { }

scoped_fd::scoped_fd(const char* filename, int mode, mode_t perm) {
  this->open(filename, mode, perm);
}

scoped_fd::scoped_fd(const string& filename, int mode, mode_t perm) {
  this->open(filename.c_str(), mode, perm);
}

scoped_fd::scoped_fd(scoped_fd&& other) : fd(other.fd) {
  other.fd = -1;
}

scoped_fd::~scoped_fd() {
  if (this->fd >= 0) {
    ::close(this->fd);
  }
}

scoped_fd& scoped_fd::operator=(int other) {
  if (this->fd >= 0) {
    ::close(this->fd);
  }
  this->fd = other;
  return *this;
}

scoped_fd::operator int() const {
  return this->fd;
}

void scoped_fd::open(const char* filename, int mode, mode_t perm) {
  this->fd = ::open(filename, mode, perm);
  if (this->fd < 0) {
    throw cannot_open_file(filename);
  }
}

void scoped_fd::open(const string& filename, int mode, mode_t perm) {
  this->open(filename.c_str(), mode, perm);
}

void scoped_fd::close() {
  if (this->fd >= 0) {
    ::close(this->fd);
    this->fd = -1;
  }
}

bool scoped_fd::is_open() {
  return this->fd >= 0;
}

void readx(int fd, void* data, size_t size) {
  if (read(fd, data, size) != (ssize_t)size) {
    throw io_error(fd);
  }
}

void writex(int fd, const void* data, size_t size) {
  if (write(fd, data, size) != (ssize_t)size) {
    throw io_error(fd);
  }
}

string readx(int fd, size_t size) {
  string ret(size, 0);
  readx(fd, const_cast<char*>(ret.data()), size);
  return ret;
}

void writex(int fd, const string& data) {
  writex(fd, data.data(), data.size());
}

void preadx(int fd, void* data, size_t size, off_t offset) {
  if (pread(fd, data, size, offset) != (ssize_t)size) {
    throw io_error(fd);
  }
}

void pwritex(int fd, const void* data, size_t size, off_t offset) {
  if (pwrite(fd, data, size, offset) != (ssize_t)size) {
    throw io_error(fd);
  }
}

string preadx(int fd, size_t size, off_t offset) {
  string ret(size, 0);
  preadx(fd, const_cast<char*>(ret.data()), size, offset);
  return ret;
}

void pwritex(int fd, const string& data, off_t offset) {
  pwritex(fd, data.data(), data.size(), offset);
}

void freadx(FILE* f, void* data, size_t size) {
  if (fread(data, 1, size, f) != size) {
    throw io_error(fileno(f));
  }
}

void fwritex(FILE* f, const void* data, size_t size) {
  if (fwrite(data, 1, size, f) != size) {
    throw io_error(fileno(f));
  }
}

string freadx(FILE* f, size_t size) {
  string ret(size, 0);
  freadx(f, const_cast<char*>(ret.data()), size);
  return ret;
}

void fwritex(FILE* f, const string& data) {
  fwritex(f, data.data(), data.size());
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

void unlink(const string& filename, bool recursive) {
  if (recursive) {
    if (isdir(filename)) {
      for (const string& item : list_directory(filename)) {
        unlink(filename + "/" + item, true);
      }
      if (rmdir(filename.c_str()) != 0) {
        throw runtime_error("can\'t delete directory " + filename + ": " + string_for_error(errno));
      }
    } else {
      unlink(filename, false);
    }

  } else {
    if (unlink(filename.c_str()) != 0) {
      throw runtime_error("can\'t delete file " + filename + ": " + string_for_error(errno));
    }
  }
}

pair<int, int> pipe() {
  int fds[2];
  if (pipe(fds)) {
    throw runtime_error("pipe failed: " + string_for_error(errno));
  }
  return make_pair(fds[0], fds[1]);
}

void make_fd_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    throw runtime_error("can\'t get socket flags: " + string_for_error(errno));
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    throw runtime_error("can\'t set socket flags: " + string_for_error(errno));
  }
}

void Poll::add(int fd, short events) {
  auto pred = [](const struct pollfd& x, const struct pollfd& y) {
    return x.fd < y.fd;
  };

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  auto insert_it = upper_bound(this->poll_fds.begin(), this->poll_fds.end(),
      pfd, pred);
  if (insert_it != this->poll_fds.end() && insert_it->fd == fd) {
    insert_it->events = events;
  } else {
    this->poll_fds.insert(insert_it, pfd);
  }
}

void Poll::remove(int fd, bool close_fd) {
  auto pred = [](const struct pollfd& x, const struct pollfd& y) {
    return x.fd < y.fd;
  };

  struct pollfd pfd;
  pfd.fd = fd;
  auto erase_it = lower_bound(this->poll_fds.begin(), this->poll_fds.end(),
      pfd, pred);
  if (erase_it != this->poll_fds.end() && erase_it->fd == fd) {
    this->poll_fds.erase(erase_it);
    if (close_fd) {
      close(fd);
    }
  }
}

unordered_map<int, short> Poll::poll(int timeout_ms) {
  if (::poll(this->poll_fds.data(), this->poll_fds.size(), timeout_ms) < 0) {
    if (errno == EINTR) {
      return unordered_map<int, short>();
    }
    throw runtime_error("poll failed: " + string_for_error(errno));
  }

  unordered_map<int, short> ret;
  for (const auto& pfd : this->poll_fds) {
    if (pfd.revents) {
      ret.emplace(pfd.fd, pfd.revents);
    }
  }
  return ret;
}
