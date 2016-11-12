#include <assert.h>
#include <unistd.h>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;


int main(int argc, char* argv[]) {
  {
    auto results = list_directory(".");
    assert(results.count("FilesystemTest") == 1);
    assert(results.count("the-test-will-fail-if-this-file-exists") == 0);
    assert(results.count(".") == 0);
    assert(results.count("..") == 0);
  }

  {
    auto filter_fn = [](struct dirent* d) -> bool {
      return ends_with(d->d_name, ".cc");
    };
    auto results = list_directory(".", filter_fn);
    assert(results.count("FilesystemTest") == 0);
    assert(results.count("Filesystem.hh") == 0);
    assert(results.count("Filesystem.cc") == 1);
  }

  {
    string filename("FilesystemTest-data");
    string symlink_name("FilesystemTest-link");
    try {
      string data("0123456789");

      save_file(filename, data);
      assert(load_file(filename) == "0123456789");

      assert(stat(filename).st_size == data.size());
      assert(lstat(filename).st_size == data.size());

      symlink(filename.c_str(), symlink_name.c_str());

      assert(stat(symlink_name).st_size == data.size());

      // the symlink size should be the length of the target
      assert(lstat(symlink_name).st_size == filename.size());

      {
        auto f = fopen_unique(filename, "r+b");
        assert(fstat(f.get()).st_size == data.size());
        assert(fstat(fileno(f.get())).st_size == data.size());
        fseek(f.get(), 5, SEEK_SET);
        fwrite(data.data(), 1, data.size(), f.get());
      }

      assert(stat(symlink_name).st_size == data.size() + 5);
      assert(load_file(symlink_name) == data.substr(0, 5) + data);

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
    writex(p.second, "omg");
    assert(readx(p.first, 3) == "omg");

    Poll poll;
    poll.add(p.first, POLLIN);
    poll.add(p.second, POLLOUT);
    unordered_map<int, short> expected_result({{p.second, POLLOUT}});
    assert(poll.poll() == expected_result);
    poll.remove(p.first, true);
    poll.remove(p.second, true);
  }

  // TODO: test get_user_home_directory

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
