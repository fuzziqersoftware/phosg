#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "Process.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;


int main(int argc, char** argv) {

  {
    const char* data = "0123456789";
    size_t data_size = strlen(data);

    auto f = popen_unique("echo 0123456789", "r");
    fwrite(data, data_size, 1, f.get());
    fflush(f.get());

    char buffer[data_size];
    expect_eq(1, fread(buffer, data_size, 1, f.get()));
    expect_eq(0, memcmp(buffer, data, data_size));
  }

  {
    string s = name_for_pid(getpid());
    expect_eq("ProcessTest", name_for_pid(getpid()));
    expect_eq(getpid(), pid_for_name("ProcessTest"));
    expect_eq(getpid(), pid_for_name("processtest"));
  }

  {
    unordered_map<pid_t, string> ret = list_processes(false);
    expect_eq("ProcessTest", ret.at(getpid()));
    ret = list_processes(true);
    expect(starts_with(ret.at(getpid()), argv[0]));
  }

  // test run_process failure
  {
    auto ret = run_process({"false"}, NULL, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(1, WEXITSTATUS(ret.exit_status));
  }

  // test run_process with stdout/stderr data
  {
    auto ret = run_process({"ls", argv[0]}, NULL, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq(string(argv[0]) + "\n", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    auto ret = run_process({"ls", string(argv[0]) + "_this_file_should_not_exist"}, NULL, false);
    expect(WIFEXITED(ret.exit_status));
    expect_ne(0, WEXITSTATUS(ret.exit_status));
    expect_eq("", ret.stdout_contents);
    expect_ne(string::npos, ret.stderr_contents.find("No such file or directory"));
  }

  // test run_process with stdin data
  {
    auto ret = run_process({"cat"}, NULL, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq("", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    string stdin_data;
    auto ret = run_process({"cat"}, &stdin_data, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq("", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    string stdin_data("1234567890");
    auto ret = run_process({"cat"}, &stdin_data, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq(stdin_data, ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    string stdin_data("2\n4\n6\n8\n1\n3\n7\n5\n9\n0\n");
    auto ret = run_process({"sort"}, &stdin_data, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }

  // TODO: test run_process timeout behavior

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
