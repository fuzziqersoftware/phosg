#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "Image.hh"
#include "Strings.hh"

int main(int argc, char** argv) {
  const char* src_filename = (argc > 1) ? argv[1] : nullptr;
  const char* dst_filename = (argc > 2) ? argv[2] : nullptr;
  if (argc > 3) {
    phosg::fwrite_fmt(stderr, "too many positional arguments given\n");
    return 1;
  }

  std::string data;
  if (!src_filename || !strcmp(src_filename, "-")) {
    data = phosg::read_all(stdin);
  } else {
    data = phosg::load_file(src_filename);
  }
  std::string png_data = phosg::ImageRGBA8888N::from_file_data(data.data(), data.size()).serialize(phosg::ImageFormat::PNG);

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    phosg::fwritex(stdout, png_data);
  } else {
    phosg::save_file(dst_filename, png_data);
  }

  return 0;
}
