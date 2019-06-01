#include <stdio.h>

#include <string>

#include "FileCache.hh"
#include "Filesystem.hh"
#include "UnitTest.hh"

using namespace std;


static void delete_test_files() {
  if (isfile("./FileCacheTest-file1")) {
    remove("./FileCacheTest-file1");
  }
  if (isfile("./FileCacheTest-file2")) {
    remove("./FileCacheTest-file2");
  }
  if (isfile("./FileCacheTest-file3")) {
    remove("./FileCacheTest-file3");
  }
  if (isfile("./FileCacheTest-file4")) {
    remove("./FileCacheTest-file4");
  }
  if (isfile("./FileCacheTest-file5")) {
    remove("./FileCacheTest-file5");
  }
}

int main(int argc, char** argv) {
  try {
    delete_test_files();

    FileCache c(3);
    expect_eq(0, c.size());
    expect_eq(false, isfile("./FileCacheTest-file1"));
    expect_eq(false, isfile("./FileCacheTest-file2"));
    expect_eq(false, isfile("./FileCacheTest-file3"));
    expect_eq(false, isfile("./FileCacheTest-file4"));
    expect_eq(false, isfile("./FileCacheTest-file5"));
    expect_eq(false, c.is_open("./FileCacheTest-file1"));
    expect_eq(false, c.is_open("./FileCacheTest-file2"));
    expect_eq(false, c.is_open("./FileCacheTest-file3"));
    expect_eq(false, c.is_open("./FileCacheTest-file4"));
    expect_eq(false, c.is_open("./FileCacheTest-file5"));

    {
      auto lease1 = c.lease("./FileCacheTest-file1", 0644);
    }
    expect_eq(true, isfile("./FileCacheTest-file1"));
    expect_eq(false, isfile("./FileCacheTest-file2"));
    expect_eq(false, isfile("./FileCacheTest-file3"));
    expect_eq(false, isfile("./FileCacheTest-file4"));
    expect_eq(false, isfile("./FileCacheTest-file5"));
    expect_eq(true, c.is_open("./FileCacheTest-file1"));
    expect_eq(false, c.is_open("./FileCacheTest-file2"));
    expect_eq(false, c.is_open("./FileCacheTest-file3"));
    expect_eq(false, c.is_open("./FileCacheTest-file4"));
    expect_eq(false, c.is_open("./FileCacheTest-file5"));

    int file2_fd;
    {
      auto lease2 = c.lease("./FileCacheTest-file2", 0644);
      file2_fd = lease2->fd;
    }
    {
      auto lease3 = c.lease("./FileCacheTest-file3", 0644);
    }
    {
      auto lease1 = c.lease("./FileCacheTest-file1", 0644);
    }
    {
      auto lease4 = c.lease("./FileCacheTest-file4", 0644);
    }
    expect_eq(true, isfile("./FileCacheTest-file1"));
    expect_eq(true, isfile("./FileCacheTest-file2"));
    expect_eq(true, isfile("./FileCacheTest-file3"));
    expect_eq(true, isfile("./FileCacheTest-file4"));
    expect_eq(false, isfile("./FileCacheTest-file5"));
    expect_eq(true, c.is_open("./FileCacheTest-file1"));
    expect_eq(false, c.is_open("./FileCacheTest-file2"));
    expect_eq(true, c.is_open("./FileCacheTest-file3"));
    expect_eq(true, c.is_open("./FileCacheTest-file4"));
    expect_eq(false, c.is_open("./FileCacheTest-file5"));

    // expect file2 to be closed, since it was least recently used
    struct stat st;
    expect_eq(-1, fstat(file2_fd, &st));
    expect_eq(EBADF, errno);

    if (isfile("./FileCacheTest-file1")) {
      remove("./FileCacheTest-file1");
    }
    if (isfile("./FileCacheTest-file2")) {
      remove("./FileCacheTest-file2");
    }
    if (isfile("./FileCacheTest-file3")) {
      remove("./FileCacheTest-file3");
    }
    if (isfile("./FileCacheTest-file4")) {
      remove("./FileCacheTest-file4");
    }

  } catch (...) {
    delete_test_files();
    throw;
  }
  delete_test_files();

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
