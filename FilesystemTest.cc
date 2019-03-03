#include <unistd.h>

#include "Filesystem.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;


int main(int argc, char* argv[]) {
  {
    auto results = list_directory(".");
    expect_eq(1, results.count("FilesystemTest"));
    expect_eq(0, results.count("the-test-will-fail-if-this-file-exists"));
    expect_eq(0, results.count("."));
    expect_eq(0, results.count(".."));
  }

  {
    auto filter_fn = [](struct dirent* d) -> bool {
      return ends_with(d->d_name, ".cc");
    };
    auto results = list_directory(".", filter_fn);
    expect_eq(0, results.count("FilesystemTest"));
    expect_eq(0, results.count("Filesystem.hh"));
    expect_eq(1, results.count("Filesystem.cc"));
  }

  {
    string filename("FilesystemTest-data");
    string symlink_name("FilesystemTest-link");
    try {
      string data("0123456789");

      save_file(filename, data);
      expect_eq("0123456789", load_file(filename));

      expect_eq(data.size(), (size_t)stat(filename).st_size);
      expect_eq(data.size(), (size_t)lstat(filename).st_size);

      symlink(filename.c_str(), symlink_name.c_str());

      expect_eq(data.size(), (size_t)stat(symlink_name).st_size);
      expect_eq(filename.size(), (size_t)lstat(symlink_name).st_size);

      {
        auto f = fopen_unique(filename, "r+b");
        expect_eq(data.size(), (size_t)fstat(f.get()).st_size);
        expect_eq(data.size(), (size_t)fstat(fileno(f.get())).st_size);
        fseek(f.get(), 5, SEEK_SET);
        fwrite(data.data(), 1, data.size(), f.get());
      }

      expect_eq(data.size() + 5, (size_t)stat(symlink_name).st_size);
      expect_eq(data.substr(0, 5) + data, load_file(symlink_name));

    } catch (...) {
      remove(filename.c_str());
      remove(symlink_name.c_str());
      throw;
    }
    remove(filename.c_str());
    remove(symlink_name.c_str());
  }

  {
    auto p = pipe();
    writex(p.second, "omg", 3);
    expect_eq("omg", readx(p.first, 3));

    Poll poll;
    poll.add(p.first, POLLIN);
    poll.add(p.second, POLLOUT);
    unordered_map<int, short> expected_result({{p.second, POLLOUT}});
    expect_eq(expected_result, poll.poll());
    poll.remove(p.first, true);
    poll.remove(p.second, true);
  }

  // TODO: test get_user_home_directory

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
