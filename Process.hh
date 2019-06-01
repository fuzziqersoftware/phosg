#pragma once

#ifndef WINDOWS

#include <stdio.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


std::unique_ptr<FILE, void(*)(FILE*)> popen_unique(const std::string& command,
    const std::string& mode);

std::string name_for_pid(pid_t pid);
pid_t pid_for_name(const std::string& name, bool search_commands = true, bool exclude_self = true);

std::unordered_map<pid_t, std::string> list_processes(bool with_commands = true);

bool pid_exists(pid_t pid);
#ifdef LINUX
bool pid_is_zombie(pid_t pid);
#endif

// returns the process' start time, in platform-dependent units. on linux, the
// value is in nanoseconds since the epoch; on osx, it's in microseconds since
// the epoch. if allow_zombie is true, returns the start time even if the
// process is a zombie. allow_zombie is ignored on osx (it's always false).
uint64_t start_time_for_pid(pid_t pid, bool allow_zombie = false);

pid_t getpid_cached();
uint64_t this_process_start_time();

class Subprocess {
public:
  Subprocess(const std::vector<std::string>& cmd, int stdin_fd = -1,
      int stdout_fd = -1, int stderr_fd = -1, const std::string* cwd = NULL,
      const std::unordered_map<std::string, std::string>* env = NULL);
  ~Subprocess();

  int stdin();
  int stdout();
  int stderr();

  pid_t pid();

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
    const std::string* stdin_data = NULL, bool check = true,
    const std::string* cwd = NULL,
    const std::unordered_map<std::string, std::string>* env = NULL,
    size_t timeout_secs = 0);

#endif
