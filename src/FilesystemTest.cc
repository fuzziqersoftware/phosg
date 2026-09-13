#include <unistd.h>

#include "Filesystem.hh"
#include "Platform.hh"
#include "Strings.hh"
#include "UnitTest.hh"

int main(int, char**) {
  {
    std::string filename("FilesystemTest-data");
    std::string symlink_name("FilesystemTest-link");
    try {
      std::string data("0123456789");

      phosg::save_file(filename, data);
      expect_eq("0123456789", phosg::load_file(filename));

#ifndef PHOSG_WINDOWS
      expect_eq(data.size(), (size_t)phosg::stat(filename).st_size);
      expect_eq(data.size(), (size_t)phosg::lstat(filename).st_size);

      symlink(filename.c_str(), symlink_name.c_str());

      expect_eq(data.size(), (size_t)phosg::stat(symlink_name).st_size);
      expect_eq(filename.size(), (size_t)phosg::lstat(symlink_name).st_size);
#endif

      {
        auto f = phosg::fopen_unique(filename, "r+b");
#ifndef PHOSG_WINDOWS
        expect_eq(data.size(), (size_t)phosg::fstat(f.get()).st_size);
        expect_eq(data.size(), (size_t)phosg::fstat(fileno(f.get())).st_size);
#endif
        fseek(f.get(), 5, SEEK_SET);
        fwrite(data.data(), 1, data.size(), f.get());
      }

#ifndef PHOSG_WINDOWS
      expect_eq(data.size() + 5, (size_t)phosg::stat(symlink_name).st_size);
      expect_eq(data.substr(0, 5) + data, phosg::load_file(symlink_name));
#endif

    } catch (...) {
      remove(filename.c_str());
#ifndef PHOSG_WINDOWS
      remove(symlink_name.c_str());
#endif
      throw;
    }
    remove(filename.c_str());
#ifndef PHOSG_WINDOWS
    remove(symlink_name.c_str());
#endif
  }

#ifndef PHOSG_WINDOWS
  {
    auto p = phosg::pipe();
    phosg::writex(p.second, "omg", 3);
    expect_eq("omg", phosg::readx(p.first, 3));

    phosg::Poll poll;
    poll.add(p.first, POLLIN);
    poll.add(p.second, POLLOUT);
    std::unordered_map<int, short> expected_result{{p.second, POLLOUT}};
    expect_eq(expected_result, poll.poll());
    poll.remove(p.first, true);
    poll.remove(p.second, true);
  }
#endif

  // TODO: test get_user_home_directory

  phosg::fwrite_fmt(stdout, "FilesystemTest: all tests passed\n");
  return 0;
}
