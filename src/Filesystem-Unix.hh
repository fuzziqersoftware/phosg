#pragma once

namespace phosg {

class cannot_stat_file : virtual public std::runtime_error {
public:
  cannot_stat_file(int fd);
  cannot_stat_file(const std::string& filename);

  int error;
};

std::string get_user_home_directory();

struct stat stat(const std::string& filename);
struct stat lstat(const std::string& filename);
struct stat fstat(int fd);
struct stat fstat(FILE* f);

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

std::unique_ptr<FILE, void (*)(FILE*)> fdopen_unique(int fd, const std::string& mode = "rb");
std::shared_ptr<FILE> fdopen_shared(int fd, const std::string& mode = "rb");
std::unique_ptr<FILE, void (*)(FILE*)> fmemopen_unique(const void* buf, size_t size);
std::shared_ptr<FILE> fmemopen_shared(const void* buf, size_t size);

std::string read_all(int fd);
std::string read(int fd, size_t size);

void readx(int fd, void* data, size_t size);
void writex(int fd, const void* data, size_t size);
std::string readx(int fd, size_t size);
void writex(int fd, const std::string& data);

void preadx(int fd, void* data, size_t size, off_t offset);
void pwritex(int fd, const void* data, size_t size, off_t offset);
std::string preadx(int fd, size_t size, off_t offset);
void pwritex(int fd, const std::string& data, off_t offset);

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

} // namespace phosg
