#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "JSON.hh"

void print_usage() {
  phosg::fwrite_fmt(stderr, "\
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
        options |= phosg::JSON::SerializeOption::FORMAT;
      } else if (!strcmp(argv[x], "--expand-leaf-containers")) {
        options |= phosg::JSON::SerializeOption::EXPAND_LEAF_CONTAINERS;
      } else if (!strcmp(argv[x], "--compress")) {
        options &= ~phosg::JSON::SerializeOption::FORMAT;
      } else if (!strcmp(argv[x], "--hex-integers")) {
        options |= phosg::JSON::SerializeOption::HEX_INTEGERS;
      } else {
        phosg::fwrite_fmt(stderr, "unknown argument: {}\n", argv[x]);
        return 1;
      }
    } else if (!src_filename) {
      src_filename = argv[x];
    } else if (!dst_filename) {
      dst_filename = argv[x];
    } else {
      phosg::fwrite_fmt(stderr, "too many positional arguments given\n");
      return 1;
    }
  }

  std::string src_data;
  if (!src_filename || !strcmp(src_filename, "-")) {
    src_data = phosg::read_all(stdin);
  } else {
    src_data = phosg::load_file(src_filename);
  }

  phosg::JSON json;
  try {
    json = phosg::JSON::parse(src_data);
  } catch (const std::exception& e) {
    phosg::fwrite_fmt(stderr, "cannot parse input: {}\n", e.what());
    return 2;
  }

  std::string result = json.serialize(options);

  if (!dst_filename || !strcmp(dst_filename, "-")) {
    phosg::fwritex(stdout, result);
  } else {
    phosg::save_file(dst_filename, result);
  }

  return 0;
}
