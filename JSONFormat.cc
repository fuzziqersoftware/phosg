#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "JSON.hh"
#include "JSONPickle.hh"

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
    size of the resulting data. No effect when --write-pickle is specified.\n\
  --read-pickle: Expect input data in pickle format instead of JSON.\n\
  --write-pickle: Write output in pickle format (protocol 2) instead of JSON.\n\
  --binary-stdout: Don\'t complain if writing pickle data to standard output\n\
    when it\'s a terminal.\n\
\n", argv0);
}


int main(int argc, char** argv) {

  bool format = true;
  bool write_pickle = false;
  bool read_pickle = false;
  bool binary_stdout = false;
  const char* src_filename = NULL;
  const char* dst_filename = NULL;
  for (int x = 1; x < argc; x++) {
    if (argv[x][0] == '-') {
      if (!strcmp(argv[x], "--help")) {
        print_usage(argv[0]);
        return 1;
      } else if (!strcmp(argv[x], "--format")) {
        format = true;
      } else if (!strcmp(argv[x], "--compress")) {
        format = false;
      } else if (!strcmp(argv[x], "--read-pickle")) {
        read_pickle = true;
      } else if (!strcmp(argv[x], "--write-pickle")) {
        write_pickle = true;
      } else if (!strcmp(argv[x], "--binary-stdout")) {
        binary_stdout = true;
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

  if (write_pickle && !binary_stdout && isatty(fileno(stdout))) {
    fprintf(stderr, "stdout is a terminal; writing pickle data will likely be unreadable\n");
    fprintf(stderr, "use --binary-stdout to write pickle data to stdout anyway\n");
    return 1;
  }

  string src_data;
  if (!src_filename || !strcmp(src_filename, "-")) {
    src_data = read_all(stdin);
  } else {
    src_data = load_file(src_filename);
  }

  shared_ptr<JSONObject> json;
  try {
    if (read_pickle) {
      json = parse_pickle(src_data);
    } else {
      json = JSONObject::parse(src_data);
    }
  } catch (const exception& e) {
    fprintf(stderr, "cannot parse input: %s\n", e.what());
    return 2;
  }

  string result;
  if (write_pickle) {
    result = serialize_pickle(*json);
  } else if (format) {
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
