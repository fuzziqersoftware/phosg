#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "Image.hh"
#include "Strings.hh"

using namespace std;

int main(int argc, char** argv) {
  const char* src_filename = (argc > 1) ? argv[1] : nullptr;
  const char* dst_filename = (argc > 2) ? argv[2] : nullptr;
  if (argc > 3) {
    fprintf(stderr, "too many positional arguments given\n");
    return 1;
  }

  Image img;
  if (!src_filename || !strcmp(src_filename, "-")) {
    img = Image(stdin);
  } else {
    img = Image(src_filename);
  }

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    img.save(stdout, Image::Format::PNG);
  } else {
    img.save(dst_filename, Image::Format::PNG);
  }

  return 0;
}
