#include "Process.hh"

#include "Platform.hh"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef PHOSG_MACOS
#include <libproc.h>
#include <sys/proc_info.h>
#else
#include <signal.h>
#endif

#include <deque>
#include <set>

#include "Filesystem.hh"
#include "Strings.hh"
#include "Time.hh"

using namespace std;

unique_ptr<FILE, void (*)(FILE*)> popen_unique(const string& command,
    const string& mode) {
  unique_ptr<FILE, void (*)(FILE*)> f(
      popen(command.c_str(), mode.c_str()),
      [](FILE* f) {
        pclose(f);
      });
  if (!f.get()) {
    throw cannot_open_file(command);
  }
  return f;
}

string name_for_pid(pid_t pid) {
  string command = string_printf("ps -ax -c -o pid -o command | grep ^\\ *%u\\ | sed s/[0-9]*\\ //g", pid);
  auto f = popen_unique(command.c_str(), "r");

  string name;

  int ch;
  while ((ch = fgetc(f.get())) != EOF) {
    if (ch >= 0x20 && ch < 0x7F) {
      name += ch;
    }
  }

  return name;
}

pid_t pid_for_name(const string& name, bool search_commands, bool exclude_self) {
  string lower_name = tolower(name);

  pid_t self_pid = exclude_self ? getpid() : 0;

  pid_t pid = 0;
  for (const auto& it : list_processes(search_commands)) {
    string lower_it_name = tolower(it.second);
    if (!strstr(lower_it_name.c_str(), lower_name.c_str())) {
      continue;
    }

    if (pid == self_pid) {
      continue;
    }
    if (pid) {
      throw runtime_error("multiple processes found");
    }
    pid = it.first;
  }

  if (pid) {
    return pid;
  }

  throw out_of_range("no processes found");
}

// calls the specified callback once for each process
unordered_map<pid_t, string> list_processes(bool with_commands) {

  auto f = popen_unique(with_commands ? "ps -ax -o pid -o command | grep [0-9]" : "ps -ax -c -o pid -o command | grep [0-9]", "r");

  unordered_map<pid_t, string> ret;
  while (!feof(f.get())) {
    pid_t pid;
    fscanf(f.get(), "%d", &pid);

    int ch;
    while ((ch = fgetc(f.get())) == ' ')
      ;
    ungetc(ch, f.get());

    string name;
    while (ch != '\n') {
      ch = fgetc(f.get());
      if (ch == EOF) {
        break;
      }
      if (ch >= 0x20 && ch < 0x7F) {
        name += (char)ch;
      }
    }

    if (!name.empty()) {
      ret.emplace(pid, name);
    }
  }

  return ret;
}

bool pid_exists(pid_t pid) {
  if (!kill(pid, 0)) {
    return true;
  }
  if (errno == ESRCH) {
    return false;
  }
  return true;
}

#ifdef PHOSG_LINUX
bool pid_is_zombie(pid_t pid) {
  // so many syscalls... sigh
  char status_data[2048]; // this is probably big enough
  try {
    string filename = string_printf("/proc/%d/status", pid);
    scoped_fd fd(filename, O_RDONLY);
    ssize_t bytes_read = read(fd, status_data, 2047);
    if (bytes_read < 0) {
      throw runtime_error("can\'t read stat file for pid " + to_string(pid));
    }
    status_data[bytes_read] = '\0';
  } catch (const cannot_open_file& e) {
    return false; // non-running processes are not zombies
  }

  char* state = strstr(status_data, "\nState:");
  if (!state) {
    return false;
  }

  state += skip_whitespace(state + 7, 0) + 7; // +7 to skip over "\nState:"
  return (*state == 'Z');
}
#endif

#ifndef PHOSG_WINDOWS
uint64_t start_time_for_pid(pid_t pid, bool allow_zombie) {
#ifdef PHOSG_MACOS
  (void)allow_zombie;

  struct proc_taskallinfo ti;
  int ret = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &ti, sizeof(ti));
  if (ret <= 0) {
    if (errno == ESRCH) {
      return 0;
    }
    throw runtime_error("can\'t get start time for pid " + to_string(pid) +
        ": " + string_for_error(errno));
  }
  if (static_cast<size_t>(ret) < sizeof(ti)) {
    throw runtime_error("can\'t get start time for pid " + to_string(pid) +
        ": " + string_for_error(errno));
  }

  return (uint64_t)ti.pbsd.pbi_start_tvsec * 1000000 +
      (uint64_t)ti.pbsd.pbi_start_tvusec;

#else
  uint64_t start_time;
  try {
    struct stat st = stat(string_printf("/proc/%d", pid));
    start_time = (uint64_t)st.st_mtim.tv_sec * 1000000000 +
        (uint64_t)st.st_mtim.tv_nsec;
  } catch (const cannot_stat_file& e) {
    return 0;
  }

  if (!allow_zombie && pid_is_zombie(pid)) {
    return 0;
  }
  return start_time;
#endif
}
#endif

static bool atfork_handler_added = false;
static pid_t cached_this_process_pid = 0;
static uint64_t cached_this_process_start_time = 0;

static void clear_cached_pid_vars() {
  cached_this_process_pid = 0;
  cached_this_process_start_time = 0;
  // TODO: do pthread_atfork() handlers survive in the child process? if not,
  // we should set atfork_handler_added to false here
}

static void maybe_add_atfork_handler() {
  if (!atfork_handler_added) {
    pthread_atfork(nullptr, nullptr, clear_cached_pid_vars);
    atfork_handler_added = true;
  }
}

pid_t getpid_cached() {
  if (!cached_this_process_pid) {
    maybe_add_atfork_handler();
    cached_this_process_pid = getpid();
  }
  return cached_this_process_pid;
}

#ifndef PHOSG_WINDOWS
uint64_t this_process_start_time() {
  if (!cached_this_process_start_time) {
    // don't need to call maybe_add_atfork_handler; getpid_cached will do it
    cached_this_process_start_time = start_time_for_pid(getpid_cached());
  }
  return cached_this_process_start_time;
}
#endif

static void replace_fd(int oldfd, int newfd) {
  if (oldfd != newfd) {
    dup2(oldfd, newfd);
    close(oldfd);
  }
}

Subprocess::Subprocess(const vector<string>& cmd, int stdin_fd, int stdout_fd,
    int stderr_fd, const string* cwd, const unordered_map<string, string>* env)
    : stdin_write_fd(-1),
      stdout_read_fd(-1),
      stderr_read_fd(-1),
      child_pid(0),
      exit_status(-1) {

  set<int> parent_fds_to_close;

  if (stdin_fd == -1) {
    auto pipefds = pipe();
    stdin_fd = pipefds.first;
    this->stdin_write_fd = pipefds.second;
    parent_fds_to_close.emplace(stdin_fd);
  }
  if (stdout_fd == -1) {
    auto pipefds = pipe();
    this->stdout_read_fd = pipefds.first;
    stdout_fd = pipefds.second;
    parent_fds_to_close.emplace(stdout_fd);
  }
  if (stderr_fd == -1) {
    auto pipefds = pipe();
    this->stderr_read_fd = pipefds.first;
    stderr_fd = pipefds.second;
    parent_fds_to_close.emplace(stderr_fd);
  }

  this->child_pid = fork();
  if (this->child_pid == -1) {
    throw runtime_error("fork failed: " + string_for_error(errno));
  }
  if (this->child_pid == 0) {
    // in child process
    replace_fd(stdin_fd, 0);
    replace_fd(stdout_fd, 1);
    replace_fd(stderr_fd, 2);
    close(this->stdin_write_fd);
    close(this->stdout_read_fd);
    close(this->stderr_read_fd);

    if (cwd) {
      chdir(cwd->c_str());
    }

    // make the argv list. this is ugly but it will be blown away by execve
    vector<const char*> argv;
    for (const string& s : cmd) {
      argv.emplace_back(s.c_str());
    }
    argv.emplace_back(nullptr);

    if (env) {
      vector<string> environ;
      vector<const char*> envp;
      for (const auto& it : *env) {
        environ.emplace_back(string_printf("%s=%s", it.first.c_str(), it.second.c_str()));
        envp.emplace_back(environ.back().c_str());
      }
      envp.emplace_back(nullptr);
      execve(cmd[0].c_str(), (char* const*)argv.data(), (char* const*)envp.data());

    } else {
      execvp(cmd[0].c_str(), (char* const*)argv.data());
    }
    // if we get here, fork() worked, but execv_() failed: exit child process without even doing cleanup
    _exit(1);
  }

  for (int fd : parent_fds_to_close) {
    close(fd);
  }
}

Subprocess::Subprocess()
    : stdin_write_fd(-1),
      stdout_read_fd(-1),
      stderr_read_fd(-1),
      child_pid(-1),
      terminated(false),
      exit_status(-1) {}

Subprocess::Subprocess(Subprocess&& other)
    : stdin_write_fd(other.stdin_write_fd),
      stdout_read_fd(other.stdout_read_fd),
      stderr_read_fd(other.stderr_read_fd),
      child_pid(other.child_pid),
      terminated(other.terminated),
      exit_status(other.exit_status) {
  other.stdin_write_fd = -1;
  other.stdout_read_fd = -1;
  other.stderr_read_fd = -1;
  other.child_pid = -1;
  other.terminated = true;
}

Subprocess& Subprocess::operator=(Subprocess&& other) {
  this->stdin_write_fd = other.stdin_write_fd;
  this->stdout_read_fd = other.stdout_read_fd;
  this->stderr_read_fd = other.stderr_read_fd;
  this->child_pid = other.child_pid;
  this->terminated = other.terminated;
  this->exit_status = other.exit_status;
  other.stdin_write_fd = -1;
  other.stdout_read_fd = -1;
  other.stderr_read_fd = -1;
  other.child_pid = -1;
  return *this;
}

Subprocess::~Subprocess() {
  if (this->child_pid >= 0 && this->wait(true) == -1) {
    this->kill(SIGKILL);
    this->wait();
  }
}

int Subprocess::stdin_fd() const {
  return this->stdin_write_fd;
}

int Subprocess::stdout_fd() const {
  return this->stdout_read_fd;
}

int Subprocess::stderr_fd() const {
  return this->stderr_read_fd;
}

pid_t Subprocess::pid() const {
  return this->child_pid;
}

string Subprocess::communicate(
    const void* stdin_data, size_t stdin_size, uint64_t timeout_usecs) {
  uint64_t deadline_usecs = timeout_usecs ? (now() + timeout_usecs) : 0;
  Poll p;
  if (stdin_size == 0) {
    close(this->stdin_write_fd);
    this->stdin_write_fd = -1;
  } else {
    p.add(this->stdin_write_fd, POLLOUT);
  }
  p.add(this->stdout_read_fd, POLLIN);

  size_t stdin_offset = 0;
  size_t stdout_bytes = 0;
  deque<string> stdout_queue;
  while ((this->wait(true) < 0) && (now() < deadline_usecs)) {
    if (p.empty()) {
      this->wait();
      break;
    }

    int timeout_ms;
    if (deadline_usecs) {
      uint64_t t = now();
      if (t < deadline_usecs) {
        timeout_ms = (deadline_usecs - t) / 1000;
      } else {
        timeout_ms = 0;
      }
    } else {
      timeout_ms = -1;
    }
    auto events = p.poll(timeout_ms);

    if (events.count(this->stdout_read_fd)) {
      stdout_queue.emplace_back(read(this->stdout_read_fd, 4096));
      if (stdout_queue.back().empty()) {
        p.remove(this->stdout_read_fd, true);
        this->stdout_read_fd = -1;
      } else {
        stdout_bytes += stdout_queue.back().size();
      }
    }
    if (events.count(this->stdin_write_fd)) {
      size_t bytes_remaining = stdin_size - stdin_offset;
      ssize_t bytes_written = write(
          this->stdin_write_fd,
          reinterpret_cast<const uint8_t*>(stdin_data) + stdin_offset,
          bytes_remaining);

      bool should_close_stdin = false;
      if (bytes_written <= 0) {
        should_close_stdin = true;
      } else {
        stdin_offset += bytes_written;
        should_close_stdin = (stdin_offset == stdin_size);
      }

      if (should_close_stdin) {
        p.remove(this->stdin_write_fd, true);
        this->stdin_write_fd = -1;
      }
    }
  }

  if (now() >= deadline_usecs) {
    // TODO: we should be a bit more polite here - send SIGTERM, wait a few
    // seconds, then send SIGKILL
    this->kill(SIGKILL);
    throw runtime_error("Subprocess::communicate timed out");
  }

  if (stdout_queue.empty()) {
    return "";
  } else if (stdout_queue.size() == 1) {
    return stdout_queue[0];
  } else {
    string ret;
    ret.reserve(stdout_bytes);
    for (const string& qi : stdout_queue) {
      ret += qi;
    }
    return ret;
  }
}

string Subprocess::communicate(const string& stdin_data, uint64_t timeout_usecs) {
  return this->communicate(stdin_data.data(), stdin_data.size(), timeout_usecs);
}

int Subprocess::wait(bool poll) {
  if (this->exit_status >= 0) {
    return this->exit_status;
  }
  for (;;) {
    int ret = waitpid(this->child_pid, &this->exit_status, poll ? WNOHANG : 0);
    if (ret == -1) {
      if (errno != EINTR) {
        throw runtime_error("waitpid failed: " + string_for_error(errno));
      }
    } else if (ret == 0) {
      return -1; // not terminated yet
    } else {
      assert(ret == this->child_pid);
      return this->exit_status;
    }
  }
}

void Subprocess::kill(int signum) {
  if (::kill(this->child_pid, signum)) {
    throw runtime_error("kill failed: " + string_for_error(errno));
  }
}

SubprocessResult::SubprocessResult() : elapsed_time(now()) {}

static const size_t READ_BLOCK_SIZE = 128 * 1024;

SubprocessResult run_process(const vector<string>& cmd, const string* stdin_data,
    bool check, const std::string* cwd,
    const std::unordered_map<std::string, std::string>* env,
    size_t timeout_usecs) {
  SubprocessResult ret;
  bool terminated = false;
  uint64_t start_time = now();

  Subprocess sp(cmd, -1, -1, -1, cwd, env);

  make_fd_nonblocking(sp.stdin_fd());
  make_fd_nonblocking(sp.stdout_fd());
  make_fd_nonblocking(sp.stderr_fd());

  struct Buffer {
    const string* buf;
    size_t offset;
    Buffer(const string* buf) : buf(buf),
                                offset(0) {}
  };
  unordered_map<int, Buffer> write_fd_to_buffer;
  unordered_map<int, string*> read_fd_to_buffer;

  Poll p;
  if (stdin_data) {
    write_fd_to_buffer.emplace(sp.stdin_fd(), stdin_data);
    p.add(sp.stdin_fd(), POLLOUT);
  } else {
    close(sp.stdin_fd());
  }
  read_fd_to_buffer.emplace(sp.stdout_fd(), &ret.stdout_contents);
  p.add(sp.stdout_fd(), POLLIN);
  read_fd_to_buffer.emplace(sp.stderr_fd(), &ret.stderr_contents);
  p.add(sp.stderr_fd(), POLLIN);

  // read/write to pipes as long as the process is running
  while ((ret.exit_status = sp.wait(true)) == -1) {
    for (const auto& pfd : p.poll(1000)) {
      if (pfd.second & POLLIN) {
        string* buf = read_fd_to_buffer.at(pfd.first);
        size_t read_offset = buf->size();
        buf->resize(read_offset + READ_BLOCK_SIZE);
        ssize_t bytes_read = read(pfd.first, buf->data() + read_offset,
            READ_BLOCK_SIZE);
        if (bytes_read > 0) {
          buf->resize(read_offset + bytes_read);
        } else if (bytes_read < 0) {
          buf->resize(read_offset);
          if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
            continue;
          }
          throw runtime_error("read failed: " + string_for_error(errno));
        } else { // bytes_read == 0; usually means the pipe is broken
          buf->resize(read_offset);
          p.remove(pfd.first, true);
          read_fd_to_buffer.erase(pfd.first);
        }
      }
      if (pfd.second & POLLOUT) {
        auto& buf = write_fd_to_buffer.at(pfd.first);
        size_t bytes_to_write = buf.buf->size() - buf.offset;
        ssize_t bytes_written = write(pfd.first,
            buf.buf->data() + buf.offset, bytes_to_write);
        if (bytes_written > 0) {
          buf.offset += bytes_written;
          if (buf.offset == buf.buf->size()) {
            p.remove(sp.stdin_fd(), true);
            write_fd_to_buffer.erase(pfd.first);
          }
        } else if (bytes_written < 0) {
          if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
            continue;
          }
          throw runtime_error("write failed: " + string_for_error(errno));
        } else { // bytes_written == 0; usually means the pipe is broken
          p.remove(pfd.first, true);
          write_fd_to_buffer.erase(pfd.first);
        }
      }
    }

    if (timeout_usecs && (start_time < now() - timeout_usecs)) {
      if (!terminated) {
        sp.kill(SIGTERM);
        terminated = true;
        timeout_usecs = 5000000;
        start_time = now();
      } else {
        sp.kill(SIGKILL);
      }
    }
  }
  ret.elapsed_time = now() - ret.elapsed_time;

  // read any leftover data after termination
  for (auto& it : read_fd_to_buffer) {
    for (;;) {
      size_t read_offset = it.second->size();
      it.second->resize(read_offset + READ_BLOCK_SIZE);
      ssize_t bytes_read = read(it.first, it.second->data() + read_offset,
          READ_BLOCK_SIZE);
      if (bytes_read > 0) {
        it.second->resize(it.second->size() - READ_BLOCK_SIZE + bytes_read);
      } else if (bytes_read < 0) {
        it.second->resize(read_offset);
        if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
          break;
        }
        throw runtime_error("read failed: " + string_for_error(errno));
      } else { // bytes_read == 0; usually means the pipe is broken
        it.second->resize(read_offset);
        break;
      }
    }
  }

  if (check && sp.wait()) {
    throw runtime_error(string_printf("command returned code %d\nstdout:\n%s\nstderr:\n%s",
        sp.wait(), ret.stdout_contents.c_str(), ret.stderr_contents.c_str()));
  }

  return ret;
}
