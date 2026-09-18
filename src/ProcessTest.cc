#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "Platform.hh"

#ifndef PHOSG_WINDOWS
#include <sys/wait.h>
#endif

#include "Process.hh"
#include "Strings.hh"
#include "UnitTest.hh"

#if !defined(PHOSG_WINDOWS) && !defined(PHOSG_SKIP_PROCESS_TEST)

int main(int, char** argv) {

  {
    phosg::fwrite_fmt(stderr, "-- popen_unique\n");
    const char* data = "0123456789";
    size_t data_size = 10;

    auto f = phosg::popen_unique("echo 0123456789", "r");
    fwrite(data, data_size, 1, f.get());
    fflush(f.get());

    char buffer[0x20];
    expect_eq(1, fread(buffer, data_size, 1, f.get()));
    expect_eq(0, memcmp(buffer, data, data_size));
  }

  {
    phosg::fwrite_fmt(stderr, "-- name_for_pid\n");
    std::string s = phosg::name_for_pid(getpid());
    expect_eq("ProcessTest", phosg::name_for_pid(getpid()));
    expect_eq(getpid(), phosg::pid_for_name("ProcessTest"));
    expect_eq(getpid(), phosg::pid_for_name("processtest"));
  }

  {
    phosg::fwrite_fmt(stderr, "-- list_processes\n");
    std::unordered_map<pid_t, std::string> ret = phosg::list_processes(false);
    expect_eq("ProcessTest", ret.at(getpid()));
    ret = phosg::list_processes(true);
    expect(ret.at(getpid()).starts_with(argv[0]));
  }

  // test run_process failure
  {
    phosg::fwrite_fmt(stderr, "-- run_process failure\n");
    auto ret = phosg::run_process({"false"}, nullptr, false);
    expect(WIFEXITED(ret.exit_status));
    expect_ne(0, WEXITSTATUS(ret.exit_status));
  }

  // test run_process with stdout/stderr data
  {
    phosg::fwrite_fmt(stderr, "-- run_process stdout data\n");
    auto ret = phosg::run_process({"ls", argv[0]}, nullptr, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq(std::string(argv[0]) + "\n", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    phosg::fwrite_fmt(stderr, "-- run_process stderr data\n");
    auto ret = phosg::run_process({"python3", "-c", "import sys; sys.stderr.write('this should go to stderr\\n'); sys.stderr.flush()"}, nullptr, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq("", ret.stdout_contents);
    expect_eq("this should go to stderr\n", ret.stderr_contents);
  }

  // test run_process with stdin data
  {
    phosg::fwrite_fmt(stderr, "-- run_process stdin missing\n");
    auto ret = phosg::run_process({"cat"}, nullptr, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq("", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    phosg::fwrite_fmt(stderr, "-- run_process stdin present but empty\n");
    std::string stdin_data;
    auto ret = phosg::run_process({"cat"}, &stdin_data, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq("", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    phosg::fwrite_fmt(stderr, "-- run_process stdin present\n");
    std::string stdin_data("1234567890");
    auto ret = phosg::run_process({"cat"}, &stdin_data, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq(stdin_data, ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }
  {
    phosg::fwrite_fmt(stderr, "-- run_process stdin present with newlines\n");
    std::string stdin_data("2\n4\n6\n8\n1\n3\n7\n5\n9\n0\n");
    auto ret = phosg::run_process({"sort"}, &stdin_data, false);
    expect(WIFEXITED(ret.exit_status));
    expect_eq(0, WEXITSTATUS(ret.exit_status));
    expect_eq("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n", ret.stdout_contents);
    expect_eq("", ret.stderr_contents);
  }

  // TODO: test run_process timeout behavior

  // test pid_exists
  {
    phosg::fwrite_fmt(stderr, "-- pid_exists\n");
    expect_eq(true, phosg::pid_exists(getpid()));
    pid_t child_pid = fork();
    if (child_pid == 0) {
      usleep(100000);
      _exit(0);
    }
    expect_eq(true, phosg::pid_exists(child_pid));
    usleep(200000);
    expect_eq(true, phosg::pid_exists(child_pid)); // it's a zombie, but still exists

    int exit_status;
    expect_eq(child_pid, waitpid(child_pid, &exit_status, 0));
    expect_eq(true, WIFEXITED(exit_status));
    expect_eq(0, WEXITSTATUS(exit_status));

    expect_eq(false, phosg::pid_exists(child_pid));
  }

  // test start_time_for_pid
  {
    phosg::fwrite_fmt(stderr, "-- start_time_for_pid\n");
    uint64_t my_start_time = phosg::start_time_for_pid(getpid());
    uint64_t cat1_start_time, cat2_start_time;
    {
      phosg::Subprocess p({"cat"});
      cat1_start_time = phosg::start_time_for_pid(p.pid());

      expect_lt(my_start_time, cat1_start_time);

      expect_eq(my_start_time, phosg::start_time_for_pid(getpid()));
      expect_eq(cat1_start_time, phosg::start_time_for_pid(p.pid()));
    }

    // make sure the cat processes don't start within the same jiffy
    usleep(100000);
    {
      phosg::Subprocess p({"cat"});
      cat2_start_time = phosg::start_time_for_pid(p.pid());

      expect_lt(my_start_time, cat2_start_time);
      expect_lt(cat1_start_time, cat2_start_time);

      expect_eq(my_start_time, phosg::start_time_for_pid(getpid()));
      expect_eq(cat2_start_time, phosg::start_time_for_pid(p.pid()));
    }

    // make sure we get the right result for zombie processes
    {
      pid_t child_pid = fork();
      if (!child_pid) {
        usleep(200000);
        _exit(0);
      }

      uint64_t child_start_time = phosg::start_time_for_pid(child_pid);
      expect_lt(my_start_time, child_start_time);

      expect_eq(my_start_time, phosg::start_time_for_pid(getpid()));
      expect_eq(child_start_time, phosg::start_time_for_pid(child_pid));

      // wait for it to terminate, then check if we can get its start time. on
      // osx, there's no way to get the start time of a zombie - it ignores
      // allow_zombie and always returns 0 if the process is a zombie
      usleep(400000);
      expect_eq(0, phosg::start_time_for_pid(child_pid, false));
#ifdef PHOSG_MACOS
      expect_eq(0, phosg::start_time_for_pid(child_pid, true));
#else
      expect_eq(child_start_time, phosg::start_time_for_pid(child_pid, true));
#endif

      // now reap the zombie and check again
      int exit_status;
      expect_eq(child_pid, wait(&exit_status));
      expect_eq(true, WIFEXITED(exit_status));
      expect_eq(0, WEXITSTATUS(exit_status));

      expect_eq(0, phosg::start_time_for_pid(child_pid, false));
      expect_eq(0, phosg::start_time_for_pid(child_pid, true));
    }
  }

  // test the cached process info functions
  {
    phosg::fwrite_fmt(stderr, "-- getpid_cached, this_process_start_time\n");
    pid_t pid = phosg::getpid_cached();
    uint64_t start_time = phosg::this_process_start_time();

    pid_t child_pid = fork();
    if (!child_pid) {
      expect_ne(pid, phosg::getpid_cached());
      expect_ne(start_time, phosg::this_process_start_time());
      _exit(0);
    }
    expect_eq(pid, phosg::getpid_cached());
    expect_eq(start_time, phosg::this_process_start_time());

    int exit_status;
    expect_eq(child_pid, waitpid(child_pid, &exit_status, 0));
    expect_eq(true, WIFEXITED(exit_status));
    expect_eq(0, WEXITSTATUS(exit_status));
  }

  phosg::fwrite_fmt(stdout, "ProcessTest: all tests passed\n");
  return 0;
}

#else // PHOSG_WINDOWS

int main(int, char**) {
  phosg::fwrite_fmt(stdout, "ProcessTest: tests do not run in this build environment\n");
  return 0;
}

#endif
