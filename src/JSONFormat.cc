#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "JSON.hh"

using namespace std;

void print_usage() {
  fprintf(stderr, "\
Usage: jsonformat [options] infile outfile\n\
\n\
If infile is - or not specified, read from standard input.\n\
If outfile is - or not specified, write to standard output.\n\
\n\
Options:\n\
  --format: Write output JSON in a human-readable format (default).\n\
  --compress: Instead of formatting in a human-readable format, minimize the\n\
      size of the resulting data.\n\
  --hex-integers: Write integers in hexadecimal format. This is a nonstandard\n\
      extension to JSON and most parsers won\'t accept it.\n\
\n");
}

int main(int argc, char** argv) {
  uint32_t options = 0;
  const char* src_filename = nullptr;
  const char* dst_filename = nullptr;
  for (int x = 1; x < argc; x++) {
    if (argv[x][0] == '-') {
      if (!strcmp(argv[x], "--help")) {
        print_usage();
        return 1;
      } else if (!strcmp(argv[x], "--format")) {
        options |= JSON::SerializeOption::FORMAT;
      } else if (!strcmp(argv[x], "--compress")) {
        options &= ~JSON::SerializeOption::FORMAT;
      } else if (!strcmp(argv[x], "--hex-integers")) {
        options |= JSON::SerializeOption::HEX_INTEGERS;
      } else {
        fprintf(stderr, "unknown argument: %s\n", argv[x]);
        return 1;
      }
    } else if (!src_filename) {
      src_filename = argv[x];
    } else if (!dst_filename) {
      dst_filename = argv[x];
    } else {
      fprintf(stderr, "too many positional arguments given\n");
      return 1;
    }
  }

  string src_data;
  if (!src_filename || !strcmp(src_filename, "-")) {
    src_data = read_all(stdin);
  } else {
    src_data = load_file(src_filename);
  }

  JSON json;
  try {
    json = JSON::parse(src_data);
  } catch (const exception& e) {
    fprintf(stderr, "cannot parse input: %s\n", e.what());
    return 2;
  }

  string result = json.serialize(options);

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    fwritex(stdout, result);
  } else {
    save_file(dst_filename, result);
  }

  return 0;
}
