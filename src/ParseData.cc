#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "Strings.hh"

int main(int argc, char** argv) {
  const char* src_filename = (argc > 1) ? argv[1] : nullptr;
  const char* dst_filename = (argc > 2) ? argv[2] : nullptr;
  if (argc > 3) {
    phosg::fwrite_fmt(stderr, "too many positional arguments given\n");
    return 1;
  }

  std::string src_data;
  if (!src_filename || !strcmp(src_filename, "-")) {
    src_data = phosg::read_all(stdin);
  } else {
    src_data = phosg::load_file(src_filename);
  }

  std::string result = phosg::parse_data_string(src_data, nullptr, phosg::ParseDataFlags::ALLOW_FILES);

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    phosg::fwritex(stdout, result);
  } else {
    phosg::save_file(dst_filename, result);
  }

  return 0;
}
