#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#include "Process.hh"
#include "Strings.hh"

using namespace std;


int main(int argc, char** argv) {

  {
    const char* data = "0123456789";
    size_t data_size = strlen(data);

    auto f = popen_unique("cat", "r+");
    fwrite(data, data_size, 1, f.get());
    fflush(f.get());

    char buffer[data_size];
    assert(fread(buffer, 10, 1, f.get()) == 1);
    assert(memcmp(buffer, data, 10) == 0);
  }

  string s = name_for_pid(getpid());
  assert(name_for_pid(getpid()) == "ProcessTest");
  assert(pid_for_name("ProcessTest") == getpid());
  assert(pid_for_name("processtest") == getpid());

  unordered_map<pid_t, string> ret = list_processes(false);
  assert(ret.at(getpid()) == "ProcessTest");
  ret = list_processes(true);
  assert(starts_with(ret.at(getpid()), argv[0]));

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
