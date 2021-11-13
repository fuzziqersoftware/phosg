#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "JSON.hh"

using namespace std;


void print_usage(const char* argv0) {
  fprintf(stderr, "\
Usage: %s [options] infile outfile\n\
\n\
If infile is - or not specified, read from standard input.\n\
If outfile is - or not specified, write to standard output.\n\
\n\
Options:\n\
  --format: Write output JSON in a human-readable format (default).\n\
  --compress: Instead of formatting in a human-readable format, minimize the\n\
    size of the resulting data.\n\
\n", argv0);
}


int main(int argc, char** argv) {

  bool format = true;
  const char* src_filename = nullptr;
  const char* dst_filename = nullptr;
  for (int x = 1; x < argc; x++) {
    if (argv[x][0] == '-') {
      if (!strcmp(argv[x], "--help")) {
        print_usage(argv[0]);
        return 1;
      } else if (!strcmp(argv[x], "--format")) {
        format = true;
      } else if (!strcmp(argv[x], "--compress")) {
        format = false;
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

  shared_ptr<JSONObject> json;
  try {
    json = JSONObject::parse(src_data);
  } catch (const exception& e) {
    fprintf(stderr, "cannot parse input: %s\n", e.what());
    return 2;
  }

  string result;
  if (format) {
    result = json->format();
  } else {
    result = json->serialize();
  }

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    fwritex(stdout, result);
  } else {
    save_file(dst_filename, result);
  }

  return 0;
}
