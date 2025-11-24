#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Arguments.hh"
#include "Filesystem.hh"
#include "JSON.hh"

using namespace std;
using namespace phosg;

void print_usage() {
  fwrite_fmt(stderr, "\
Usage: bindiff [options] file1 file2\n\
\n\
If either file1 or file2 is -, read from standard input. If both are -,\n\
bindiffexits immediately with no output.\n\
\n\
Options:\n\
  --help: You're reading it now.\n\
  --context=N: Show N lines of matching bytes around each differing byte.\n\
      (Default 3)\n\
  --color: Highlight differing bytes even if the output is not a TTY.\n\
  --no-color: Don't highlight differing bytes even if the output is a TTY.\n\
  --start-address=ADDR: Address the first byte as ADDR (hex) instead of 0.\n\
\n");
}

int main(int argc, char** argv) {
  Arguments args(argv, argc);
  if (args.get<bool>("help")) {
    print_usage();
    return 0;
  }

  auto filename1 = args.get<string>(1);
  auto filename2 = args.get<string>(2);
  if (filename1 == filename2) {
    return 0;
  }

  bool use_color;
  if (args.get<bool>("no-color")) {
    use_color = false;
  } else if (args.get<bool>("color")) {
    use_color = true;
  } else {
    use_color = isatty(fileno(stdout));
  }
  size_t context_lines = args.get<size_t>("context", 3);
  uint64_t base_offset = args.get<uint64_t>("start-address", 0, phosg::Arguments::IntFormat::HEX);

  string data1 = (filename1 == "-") ? read_all(stdin) : load_file(filename1);
  string data2 = (filename2 == "-") ? read_all(stdin) : load_file(filename2);

  print_binary_diff(stdout, data1.data(), data1.size(), data2.data(), data2.size(), use_color, context_lines, base_offset);
  return 0;
}
