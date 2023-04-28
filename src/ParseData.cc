#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;

int main(int argc, char** argv) {
  const char* src_filename = (argc > 1) ? argv[1] : nullptr;
  const char* dst_filename = (argc > 2) ? argv[2] : nullptr;
  if (argc > 3) {
    fprintf(stderr, "too many positional arguments given\n");
    return 1;
  }

  string src_data;
  if (!src_filename || !strcmp(src_filename, "-")) {
    src_data = read_all(stdin);
  } else {
    src_data = load_file(src_filename);
  }

  string result = parse_data_string(src_data);

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    fwritex(stdout, result);
  } else {
    save_file(dst_filename, result);
  }

  return 0;
}
