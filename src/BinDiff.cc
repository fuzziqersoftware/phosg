#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Arguments.hh"
#include "Filesystem.hh"
#include "JSON.hh"

using namespace std;
using namespace phosg;

void print_binary_diff(
    FILE* stream,
    const void* data1v,
    size_t size1,
    const void* data2v,
    size_t size2,
    bool use_color,
    size_t context_lines,
    uint64_t base_offset = 0) {
  const uint8_t* data1 = reinterpret_cast<const uint8_t*>(data1v);
  const uint8_t* data2 = reinterpret_cast<const uint8_t*>(data2v);

  size_t max_data_size = std::max<size_t>(size1, size2);
  int offset_width_digits;
  if (base_offset + max_data_size > 0x100000000) {
    offset_width_digits = 16;
  } else if (base_offset + max_data_size > 0x10000) {
    offset_width_digits = 8;
  } else if (base_offset + max_data_size > 0x100) {
    offset_width_digits = 4;
  } else {
    offset_width_digits = 2;
  }

  auto print_diff_line = [&](char left_ch,
                             const uint8_t* data,
                             size_t size,
                             size_t line_index,
                             uint16_t diff_flags,
                             TerminalFormat color) {
    size_t line_start_offset = line_index * 0x10;
    if (use_color) {
      print_color_escape(stream, color, TerminalFormat::END);
    }
    uint64_t address = base_offset + line_start_offset;
    fprintf(stream, "%c %0*" PRIX64 " |", left_ch, offset_width_digits, address);
    for (size_t within_line_offset = 0; within_line_offset < 0x10; within_line_offset++) {
      size_t offset = (line_index * 0x10) + within_line_offset;
      if (offset < size) {
        if (use_color && (diff_flags & (1 << within_line_offset))) {
          print_color_escape(stream, color, TerminalFormat::BOLD, TerminalFormat::END);
          fprintf(stream, " %02hhX", data[offset]);
          print_color_escape(stream, TerminalFormat::NORMAL, color, TerminalFormat::END);
        } else {
          fprintf(stream, " %02hhX", data[offset]);
        }
      } else {
        fprintf(stream, "   ");
      }
    }
    fprintf(stream, " | ");
    for (size_t within_line_offset = 0; within_line_offset < 0x10; within_line_offset++) {
      size_t offset = (line_index * 0x10) + within_line_offset;
      if (offset < size) {
        char ch = data[offset];
        if (ch < 0x20 || ch > 0x7E) {
          ch = ' ';
        }
        if (use_color && (diff_flags & (1 << within_line_offset))) {
          print_color_escape(stream, color, TerminalFormat::BOLD, TerminalFormat::END);
          fputc(ch, stream);
          print_color_escape(stream, TerminalFormat::NORMAL, color, TerminalFormat::END);
        } else {
          fputc(ch, stream);
        }
      } else {
        fputc(' ', stream);
      }
    }
    if (use_color) {
      print_color_escape(stream, TerminalFormat::NORMAL, TerminalFormat::END);
    }
    fputc('\n', stream);
  };

  auto print_diff_line_pair = [&](size_t line_index, uint16_t diff_flags) -> void {
    size_t line_start_offset = line_index * 0x10;
    if (diff_flags == 0) {
      print_diff_line(' ', data1, size1, line_index, diff_flags, TerminalFormat::NORMAL);
    } else {
      if (line_start_offset < size1) {
        print_diff_line('-', data1, size1, line_index, diff_flags, TerminalFormat::FG_RED);
      }
      if (line_start_offset < size2) {
        print_diff_line('+', data2, size2, line_index, diff_flags, TerminalFormat::FG_GREEN);
      }
    }
  };

  size_t first_unprinted_line_index = 0;
  ssize_t last_different_line_index = -(context_lines + 1);
  size_t num_lines = ((max_data_size + 0x0F) >> 4);
  for (size_t line_index = 0; line_index < num_lines; line_index++) {
    uint16_t diff_flags = 0;
    for (size_t within_line_offset = 0; within_line_offset < 0x10; within_line_offset++) {
      size_t offset = (line_index * 0x10) + within_line_offset;
      uint16_t data1_value = (offset < size1) ? static_cast<uint8_t>(data1[offset]) : 0xFFFF;
      uint16_t data2_value = (offset < size2) ? static_cast<uint8_t>(data2[offset]) : 0xFFFF;
      if (data1_value != data2_value) {
        diff_flags |= (1 << within_line_offset);
      }
    }
    if (diff_flags == 0) {
      if (static_cast<ssize_t>(line_index) <= last_different_line_index + static_cast<ssize_t>(context_lines)) {
        print_diff_line_pair(line_index, diff_flags);
        first_unprinted_line_index = line_index + 1;
      }

    } else {
      bool has_unprinted_gap;
      size_t chunk_start_line_index;
      if ((first_unprinted_line_index + context_lines) >= line_index) {
        chunk_start_line_index = first_unprinted_line_index;
        has_unprinted_gap = false;
      } else {
        chunk_start_line_index = line_index - context_lines;
        has_unprinted_gap = true;
      }

      if (has_unprinted_gap) {
        fprintf(stream, "  ...\n");
      }

      for (size_t z = chunk_start_line_index; z < line_index; z++) {
        print_diff_line_pair(z, 0x0000);
      }
      print_diff_line_pair(line_index, diff_flags);
      first_unprinted_line_index = line_index + 1;
      last_different_line_index = line_index;
    }
  }

  if (first_unprinted_line_index < num_lines) {
    fprintf(stream, "  ...\n");
  }
}

void print_usage() {
  fprintf(stderr, "\
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
