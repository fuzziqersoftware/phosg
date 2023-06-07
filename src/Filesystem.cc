#include "Filesystem.hh"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Platform.hh"

#include <dirent.h>
#include <poll.h>
#include <pwd.h>

#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include "Strings.hh"

using namespace std;

std::string basename(const std::string& filename) {
  size_t slash_pos = filename.rfind('/');
  return (slash_pos == string::npos) ? filename : filename.substr(slash_pos + 1);
}

std::string dirname(const std::string& filename) {
  size_t slash_pos = filename.rfind('/');
  return (slash_pos == string::npos) ? "" : filename.substr(0, slash_pos);
}

unordered_set<string> list_directory(const string& dirname) {
  unordered_set<string> files;

  DIR* dir = opendir(dirname.c_str());
  if (dir == nullptr) {
    throw cannot_open_file(dirname);
  }

  struct dirent* entry;
  while ((entry = readdir(dir))) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
      continue;
    }
    files.emplace(entry->d_name);
  }

  closedir(dir);

  return files;
}

vector<string> list_directory_sorted(const string& dirname) {
  vector<string> files;

  DIR* dir = opendir(dirname.c_str());
  if (dir == nullptr) {
    throw cannot_open_file(dirname);
  }

  struct dirent* entry;
  while ((entry = readdir(dir))) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
      continue;
    }
    files.emplace_back(entry->d_name);
  }

  closedir(dir);

  sort(files.begin(), files.end());
  return files;
}

string getcwd() {
  string ret(MAXPATHLEN, '\0');
  if (!getcwd(ret.data(), ret.size())) {
    throw runtime_error("cannot get working directory");
  }
  ret.resize(strlen(ret.c_str()));
  return ret;
}

#ifndef PHOSG_WINDOWS

// TODO: this can definitely be implemented on windows; I'm just lazy
string get_user_home_directory() {
  const char* homedir = getenv("HOME");
  if (homedir) {
    return homedir;
  }

  int bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1) {
    throw runtime_error("can\'t get _SC_GETPW_R_SIZE_MAX");
  }

  string buffer(bufsize, '\0');
  struct passwd pwd, *result = nullptr;
  if (getpwuid_r(getuid(), &pwd, buffer.data(), bufsize, &result) != 0 || !result) {
    throw runtime_error("can\'t get home directory for current user");
  }

  return pwd.pw_dir;
}

#endif

cannot_stat_file::cannot_stat_file(int fd)
    : runtime_error("can\'t stat fd " + to_string(fd) + ": " + string_for_error(errno)),
      error(errno) {}

cannot_stat_file::cannot_stat_file(const string& filename)
    : runtime_error("can\'t stat file " + filename + ": " + string_for_error(errno)),
      error(errno) {}

cannot_open_file::cannot_open_file(int fd)
    : runtime_error("can\'t open fd " + to_string(fd) + ": " + string_for_error(errno)),
      error(errno) {}

cannot_open_file::cannot_open_file(const string& filename)
    : runtime_error("can\'t open file " + filename + ": " + string_for_error(errno)),
      error(errno) {}

io_error::io_error(int fd)
    : runtime_error("io error on fd " + to_string(fd) + ": " + string_for_error(errno)),
      error(errno) {}

io_error::io_error(int fd, const string& what)
    : runtime_error(string_printf("io error on fd %d: %s", fd, what.c_str())),
      error(-1) {}

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
  ssize_t length = readlink(filename.c_str(), data.data(), data.size());
  if (length < 0) {
    throw cannot_stat_file(filename);
  }
  data.resize(length);
  data.shrink_to_fit();
  return data;
}

string realpath(const string& path) {
  string data(PATH_MAX, 0);
  if (!realpath(path.c_str(), data.data())) {
    throw cannot_stat_file(path);
  }
  data.resize(strlen(data.c_str()));
  data.shrink_to_fit();
  return data;
}

scoped_fd::scoped_fd() : fd(-1) {}

scoped_fd::scoped_fd(int fd) : fd(fd) {}

scoped_fd::scoped_fd(const char* filename, int mode, mode_t perm) : fd(-1) {
  this->open(filename, mode, perm);
}

scoped_fd::scoped_fd(const string& filename, int mode, mode_t perm) : fd(-1) {
  this->open(filename.c_str(), mode, perm);
}

scoped_fd::scoped_fd(scoped_fd&& other) : fd(other.fd) {
  other.fd = -1;
}

scoped_fd::~scoped_fd() {
  this->close();
}

scoped_fd& scoped_fd::operator=(scoped_fd&& other) {
  this->close();
  this->fd = other.fd;
  other.fd = -1;
  return *this;
}

scoped_fd& scoped_fd::operator=(int other) {
  this->close();
  this->fd = other;
  return *this;
}

scoped_fd::operator int() const {
  return this->fd;
}

void scoped_fd::open(const char* filename, int mode, mode_t perm) {
  this->close();
  this->fd = ::open(filename, mode, perm);
  if (this->fd < 0) {
    throw cannot_open_file(filename);
  }
}

void scoped_fd::open(const string& filename, int mode, mode_t perm) {
  this->close();
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

string read_all(int fd) {
  static const ssize_t read_size = 16 * 1024;

  size_t total_size = 0;
  vector<string> buffers;
  for (;;) {
    buffers.emplace_back(read_size, 0);
    ssize_t bytes_read = read(fd, buffers.back().data(), read_size);
    if (bytes_read < 0) {
      throw io_error(fd);
    }

    total_size += bytes_read;
    if (bytes_read < read_size) {
      buffers.back().resize(bytes_read);
      break;
    }
  }

  if (buffers.size() == 1) {
    return buffers.back();
  }

  string ret;
  ret.reserve(total_size);
  for (const string& buffer : buffers) {
    ret += buffer;
  }

  return ret;
}

string read_all(FILE* f) {
  static const ssize_t read_size = 16 * 1024;

  size_t total_size = 0;
  vector<string> buffers;
  for (;;) {
    buffers.emplace_back(read_size, 0);
    ssize_t bytes_read = fread(buffers.back().data(), 1, read_size, f);
    if (bytes_read < 0) {
      throw io_error(fileno(f));
    }

    total_size += bytes_read;
    if (bytes_read < read_size) {
      buffers.back().resize(bytes_read);
      break;
    }
  }

  if (buffers.size() == 1) {
    return buffers.back();
  }

  string ret;
  ret.reserve(total_size);
  for (const string& buffer : buffers) {
    ret += buffer;
  }

  return ret;
}

string read(int fd, size_t size) {
  string data(size, '\0');
  ssize_t ret_size = read(fd, data.data(), size);
  if (ret_size < 0) {
    throw io_error(fd);
  } else if (ret_size != static_cast<ssize_t>(size)) {
    data.resize(ret_size);
  }
  return data;
}

string fread(FILE* f, size_t size) {
  string data(size, '\0');
  ssize_t ret_size = fread(data.data(), 1, size, f);
  if (ret_size < 0) {
    throw io_error(fileno(f));
  } else if (ret_size != static_cast<ssize_t>(size)) {
    data.resize(ret_size);
  }
  return data;
}

void readx(int fd, void* data, size_t size) {
  ssize_t ret_size = read(fd, data, size);
  if (ret_size < 0) {
    throw io_error(fd);
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fd, string_printf("expected %zu bytes, read %zd bytes", size, ret_size));
  }
}

void writex(int fd, const void* data, size_t size) {
  ssize_t ret_size = write(fd, data, size);
  if (ret_size < 0) {
    throw io_error(fd);
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fd, string_printf("expected %zu bytes, wrote %zd bytes", size, ret_size));
  }
}

string readx(int fd, size_t size) {
  string ret(size, 0);
  readx(fd, ret.data(), size);
  return ret;
}

void writex(int fd, const string& data) {
  writex(fd, data.data(), data.size());
}

void preadx(int fd, void* data, size_t size, off_t offset) {
  ssize_t ret_size = pread(fd, data, size, offset);
  if (ret_size < 0) {
    throw io_error(fd);
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fd, string_printf("expected %zu bytes, read %zd bytes", size, ret_size));
  }
}

void pwritex(int fd, const void* data, size_t size, off_t offset) {
  ssize_t ret_size = pwrite(fd, data, size, offset);
  if (ret_size < 0) {
    throw io_error(fd);
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fd, string_printf("expected %zu bytes, wrote %zd bytes", size, ret_size));
  }
}

string preadx(int fd, size_t size, off_t offset) {
  string ret(size, 0);
  preadx(fd, ret.data(), size, offset);
  return ret;
}

void pwritex(int fd, const string& data, off_t offset) {
  pwritex(fd, data.data(), data.size(), offset);
}

void freadx(FILE* f, void* data, size_t size) {
  ssize_t ret_size = fread(data, 1, size, f);
  if (ret_size < 0) {
    throw io_error(fileno(f));
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fileno(f), string_printf("expected %zu bytes, read %zd bytes", size, ret_size));
  }
}

void fwritex(FILE* f, const void* data, size_t size) {
  ssize_t ret_size = fwrite(data, 1, size, f);
  if (ret_size < 0) {
    throw io_error(fileno(f));
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fileno(f), string_printf("expected %zu bytes, wrote %zd bytes", size, ret_size));
  }
}

string freadx(FILE* f, size_t size) {
  string ret(size, 0);
  freadx(f, ret.data(), size);
  return ret;
}

void fwritex(FILE* f, const string& data) {
  fwritex(f, data.data(), data.size());
}

uint8_t fgetcx(FILE* f) {
  int ret = fgetc(f);
  if (ret == EOF) {
    if (feof(f)) {
      throw io_error(fileno(f), "end of stream");
    } else {
      throw io_error(fileno(f), "cannot read from stream");
    }
  }
  return ret;
}

std::string fgets(FILE* f) {
  deque<std::string> blocks;
  for (;;) {
    std::string& block = blocks.emplace_back(0x100, '\0');
    if (!fgets(block.data(), block.size(), f)) {
      blocks.pop_back();
      if (feof(f)) {
        break;
      } else {
        throw io_error(fileno(f), "cannot read from stream");
      }
    }
    size_t block_bytes = strlen(block.c_str());
    if ((block_bytes < 0x100) || (block[0xFF] == '\n')) {
      block.resize(block_bytes);
      break; // The line ends at the end of this block
    }
  }

  if (blocks.size() == 1) {
    return std::move(blocks.front());
  } else {
    return join(blocks);
  }
}

string load_file(const string& filename) {
  scoped_fd fd(filename, O_RDONLY);
  ssize_t file_size = fstat(fd).st_size;

  string data(file_size, 0);
  ssize_t bytes_read = read(fd, data.data(), data.size());
  if (bytes_read != file_size) {
    if (errno == 0) {
      throw runtime_error(string_printf("can\'t read from %s: %zd/%zd bytes read",
          filename.c_str(), bytes_read, file_size));
    } else {
      throw runtime_error("can\'t read from " + filename + ": " + string_for_error(errno));
    }
  }

  return data;
}

void save_file(const string& filename, const void* data, size_t size) {
  scoped_fd fd(filename, O_CREAT | O_TRUNC | O_WRONLY);
  ssize_t bytes_written = write(fd, data, size);
  if (bytes_written != static_cast<ssize_t>(size)) {
    if (errno == 0) {
      throw runtime_error(string_printf("can\'t write to %s: %zd/%zd bytes written",
          filename.c_str(), bytes_written, size));
    } else {
      throw runtime_error("can\'t write to " + filename + ": " + string_for_error(errno));
    }
  }
}

void save_file(const string& filename, const string& data) {
  save_file(filename, data.data(), data.size());
}

static FILE* fopen_binary_raw(const string& filename, const string& mode) {
  string new_mode = mode;
  if (new_mode.find('b') == string::npos) {
    new_mode += 'b';
  }
  FILE* f = fopen(filename.c_str(), new_mode.c_str());
  if (!f) {
    throw cannot_open_file(filename);
  }
  return f;
}

static FILE* fdopen_binary_raw(int fd, const string& mode) {
  string new_mode = mode;
  if (new_mode.find('b') == string::npos) {
    new_mode += 'b';
  }
  FILE* f = fdopen(fd, new_mode.c_str());
  if (!f) {
    throw cannot_open_file(fd);
  }
  return f;
}

static void fclose_raw(FILE* f) {
  fclose(f);
}

unique_ptr<FILE, void (*)(FILE*)> fopen_unique(const string& filename,
    const string& mode, FILE* dash_file) {
  if (dash_file && (filename == "-")) {
    return unique_ptr<FILE, void (*)(FILE*)>(
        dash_file, +[](FILE*) {});
  }
  return unique_ptr<FILE, void (*)(FILE*)>(fopen_binary_raw(filename, mode), fclose_raw);
}

unique_ptr<FILE, void (*)(FILE*)> fdopen_unique(int fd, const string& mode) {
  return unique_ptr<FILE, void (*)(FILE*)>(fdopen_binary_raw(fd, mode), fclose_raw);
}

unique_ptr<FILE, void (*)(FILE*)> fmemopen_unique(const void* buf, size_t size) {
  return unique_ptr<FILE, void (*)(FILE*)>(fmemopen(
                                               const_cast<void*>(buf), size, "rb"),
      fclose_raw);
}

shared_ptr<FILE> fopen_shared(const string& filename, const string& mode,
    FILE* dash_file) {
  if (dash_file && (filename == "-")) {
    return shared_ptr<FILE>(dash_file, [](FILE*) {});
  }
  return shared_ptr<FILE>(fopen_binary_raw(filename, mode), fclose_raw);
}

shared_ptr<FILE> fdopen_shared(int fd, const string& mode) {
  return shared_ptr<FILE>(fdopen_binary_raw(fd, mode), fclose_raw);
}

void rename(const std::string& old_filename, const std::string& new_filename) {
  if (rename(old_filename.c_str(), new_filename.c_str()) != 0) {
    throw runtime_error("can\'t rename file " + old_filename + " to " + new_filename + ": " + string_for_error(errno));
  }
}

void unlink(const string& filename, bool recursive) {
  if (recursive) {
    if (isdir(filename)) {
      for (const string& item : list_directory(filename)) {
        unlink(filename + "/" + item, true);
      }
      if ((rmdir(filename.c_str()) != 0) && (errno != ENOENT)) {
        throw runtime_error("can\'t delete directory " + filename + ": " + string_for_error(errno));
      }
    } else {
      unlink(filename, false);
    }

  } else {
    if ((unlink(filename.c_str()) != 0) && (errno != ENOENT)) {
      throw runtime_error("can\'t delete file " + filename + ": " + string_for_error(errno));
    }
  }
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

pair<int, int> pipe() {
  int fds[2];
  if (pipe(fds)) {
    throw runtime_error("pipe failed: " + string_for_error(errno));
  }
  return make_pair(fds[0], fds[1]);
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

bool Poll::empty() const {
  return this->poll_fds.empty();
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
