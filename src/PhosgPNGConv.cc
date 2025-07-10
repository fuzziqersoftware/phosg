#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "Image.hh"
#include "Strings.hh"

using namespace std;
using namespace phosg;

int main(int argc, char** argv) {
  const char* src_filename = (argc > 1) ? argv[1] : nullptr;
  const char* dst_filename = (argc > 2) ? argv[2] : nullptr;
  if (argc > 3) {
    fwrite_fmt(stderr, "too many positional arguments given\n");
    return 1;
  }

  std::string data;
  if (!src_filename || !strcmp(src_filename, "-")) {
    data = read_all(stdin);
  } else {
    data = load_file(src_filename);
  }
  string png_data = ImageRGBA8888N::from_file_data(data.data(), data.size()).serialize(ImageFormat::PNG);

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    fwritex(stdout, png_data);
  } else {
    save_file(dst_filename, png_data);
  }

  return 0;
}
