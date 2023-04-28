#pragma once

#include "Platform.hh"

#include <stdio.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::unique_ptr<FILE, void (*)(FILE*)> popen_unique(const std::string& command,
    const std::string& mode);

std::string name_for_pid(pid_t pid);
pid_t pid_for_name(const std::string& name, bool search_commands = true, bool exclude_self = true);

std::unordered_map<pid_t, std::string> list_processes(bool with_commands = true);

bool pid_exists(pid_t pid);
#ifdef PHOSG_LINUX
bool pid_is_zombie(pid_t pid);
#endif

#ifndef PHOSG_WINDOWS
// returns the process' start time, in platform-dependent units. on linux, the
// value is in nanoseconds since the epoch; on osx, it's in microseconds since
// the epoch. if allow_zombie is true, returns the start time even if the
// process is a zombie. allow_zombie is ignored on osx (it's always false).
uint64_t start_time_for_pid(pid_t pid, bool allow_zombie = false);
uint64_t this_process_start_time();
#endif

pid_t getpid_cached();

class Subprocess {
public:
  Subprocess();
  explicit Subprocess(const std::vector<std::string>& cmd, int stdin_fd = -1,
      int stdout_fd = -1, int stderr_fd = -1, const std::string* cwd = nullptr,
      const std::unordered_map<std::string, std::string>* env = nullptr);
  Subprocess(const Subprocess&) = delete;
  Subprocess(Subprocess&&);
  Subprocess& operator=(const Subprocess&) = delete;
  Subprocess& operator=(Subprocess&&);
  ~Subprocess();

  int stdin_fd() const;
  int stdout_fd() const;
  int stderr_fd() const;

  pid_t pid() const;

  std::string communicate(
      const void* stdin_data = nullptr,
      size_t stdin_size = 0,
      uint64_t timeout_usecs = 0);
  std::string communicate(
      const std::string& stdin_data = "",
      uint64_t timeout_usecs = 0);

  int wait(bool poll = false);

  void kill(int signum);

protected:
  int stdin_write_fd;
  int stdout_read_fd;
  int stderr_read_fd;

  pid_t child_pid;
  bool terminated;
  int exit_status;
};

struct SubprocessResult {
  std::string stdout_contents;
  std::string stderr_contents;
  int exit_status;
  uint64_t elapsed_time;

  SubprocessResult();
};

SubprocessResult run_process(const std::vector<std::string>& cmd,
    const std::string* stdin_data = nullptr, bool check = true,
    const std::string* cwd = nullptr,
    const std::unordered_map<std::string, std::string>* env = nullptr,
    size_t timeout_secs = 0);
