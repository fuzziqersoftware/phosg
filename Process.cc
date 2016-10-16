#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Filesystem.hh"
#include "Process.hh"
#include "Strings.hh"

using namespace std;


unique_ptr<FILE, void(*)(FILE*)> popen_unique(const string& command,
    const string& mode) {
  unique_ptr<FILE, void(*)(FILE*)> f(
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

  pid_t self_pid = exclude_self ? getpid() : 0;

  pid_t pid = 0;
  for (const auto& it : list_processes(search_commands)) {
    if (!strcasestr(it.second.c_str(), name.c_str())) {
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

  auto f = popen_unique(with_commands ? "ps -ax -o pid -o command | grep [0-9]" :
      "ps -ax -c -o pid -o command | grep [0-9]", "r");

  unordered_map<pid_t, string> ret;
  while (!feof(f.get())) {
    pid_t pid;
    fscanf(f.get(), "%u", &pid);

    int ch;
    while ((ch = fgetc(f.get())) == ' ');
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
