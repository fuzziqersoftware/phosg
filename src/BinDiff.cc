#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Filesystem.hh"
#include "JSON.hh"

using namespace std;

void print_usage() {
  fprintf(stderr, "\
Usage: bindiff file1 file2\n\
\n\
If either file1 or file2 is -, read from standard input. At most one can be -.\n\
\n");
}

int main(int argc, char** argv) {
  const char* filename1 = nullptr;
  const char* filename2 = nullptr;
  for (int x = 1; x < argc; x++) {
    if (argv[x][0] == '-') {
      if (!strcmp(argv[x], "--help")) {
        print_usage();
        return 1;
      } else {
        throw runtime_error(string_printf("unknown argument: %s\n", argv[x]));
      }
    } else if (!filename1) {
      filename1 = argv[x];
    } else if (!filename2) {
      filename2 = argv[x];
    } else {
      throw runtime_error("too many positional arguments given");
    }
  }

  if (!filename1 || !filename2) {
    throw runtime_error("two filenames are required");
  }

  string data1 = (strcmp(filename1, "-") == 0) ? read_all(stdin) : load_file(filename1);
  string data2 = (strcmp(filename2, "-") == 0) ? read_all(stdin) : load_file(filename2);
  if (data1 == data2) {
    return 0;
  }

  auto lines1 = split(format_data(data1), '\n');
  auto lines2 = split(format_data(data2), '\n');

  fprintf(stdout, "--- %s\n", filename1);
  fprintf(stdout, "+++ %s\n", filename1);
  for (size_t z = 0; z < min<size_t>(lines1.size(), lines2.size()); z++) {
    if (lines1[z] != lines2[z]) {
      fprintf(stdout, "-%s\n", lines1[z].c_str());
      fprintf(stdout, "+%s\n", lines2[z].c_str());
    } else {
      fprintf(stdout, " %s\n", lines1[z].c_str());
    }
  }

  return 4;
}
