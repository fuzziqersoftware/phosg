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

  {
    string s = name_for_pid(getpid());
    assert(name_for_pid(getpid()) == "ProcessTest");
    assert(pid_for_name("ProcessTest") == getpid());
    assert(pid_for_name("processtest") == getpid());
  }

  {
    unordered_map<pid_t, string> ret = list_processes(false);
    assert(ret.at(getpid()) == "ProcessTest");
    ret = list_processes(true);
    assert(starts_with(ret.at(getpid()), argv[0]));
  }

  // test run_process failure
  {
    auto ret = run_process({"false"}, NULL, false);
    assert(WIFEXITED(ret.exit_status));
    assert(WEXITSTATUS(ret.exit_status) == 1);
  }

  // test run_process with stdout/stderr data
  {
    auto ret = run_process({"ls", argv[0]}, NULL, false);
    assert(WIFEXITED(ret.exit_status));
    assert(WEXITSTATUS(ret.exit_status) == 0);
    assert(ret.stdout_contents == string(argv[0]) + "\n");
    assert(ret.stderr_contents == "");
  }
  {
    auto ret = run_process({"ls", string(argv[0]) + "_this_file_should_not_exist"}, NULL, false);
    assert(WIFEXITED(ret.exit_status));
    assert(WEXITSTATUS(ret.exit_status) == 1);
    assert(ret.stdout_contents == "");
    assert(ret.stderr_contents.find("No such file or directory") != string::npos);
  }

  // test run_process with stdin data
  {
    auto ret = run_process({"cat"}, NULL, false);
    assert(WIFEXITED(ret.exit_status));
    assert(WEXITSTATUS(ret.exit_status) == 0);
    assert(ret.stdout_contents == "");
    assert(ret.stderr_contents == "");
  }
  {
    string stdin_data;
    auto ret = run_process({"cat"}, &stdin_data, false);
    assert(WIFEXITED(ret.exit_status));
    assert(WEXITSTATUS(ret.exit_status) == 0);
    assert(ret.stdout_contents == "");
    assert(ret.stderr_contents == "");
  }
  {
    string stdin_data("1234567890");
    auto ret = run_process({"cat"}, &stdin_data, false);
    assert(WIFEXITED(ret.exit_status));
    assert(WEXITSTATUS(ret.exit_status) == 0);
    assert(ret.stdout_contents == stdin_data);
    assert(ret.stderr_contents == "");
  }
  {
    string stdin_data("2\n4\n6\n8\n1\n3\n7\n5\n9\n0\n");
    auto ret = run_process({"sort"}, &stdin_data, false);
    assert(WIFEXITED(ret.exit_status));
    assert(WEXITSTATUS(ret.exit_status) == 0);
    assert(ret.stdout_contents == "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n");
    assert(ret.stderr_contents == "");
  }

  // TODO: test run_process timeout behavior

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
