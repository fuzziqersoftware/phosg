#include "Filesystem.hh"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Platform.hh"

#include <dirent.h>
#ifndef PHOSG_WINDOWS
#include <poll.h>
#include <pwd.h>
#endif

#include <algorithm>
#include <deque>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include "Strings.hh"

namespace phosg {

cannot_open_file::cannot_open_file(int fd)
    : runtime_error(std::format("can\'t open fd {}: {}", fd, string_for_error(errno))), error(errno) {}

cannot_open_file::cannot_open_file(const std::string& filename)
    : runtime_error(std::format("can\'t open file {}: {}", filename, string_for_error(errno))), error(errno) {}

io_error::io_error(int fd)
    : runtime_error(std::format("io error on fd {}: {}", fd, string_for_error(errno))), error(errno) {}

io_error::io_error(int fd, const std::string& what)
    : runtime_error(std::format("io error on fd {}: {}", fd, what.c_str())), error(-1) {}

std::string basename(const std::string& filename) {
  size_t slash_pos = filename.rfind('/');
  return (slash_pos == std::string::npos) ? filename : filename.substr(slash_pos + 1);
}

std::string dirname(const std::string& filename) {
  size_t slash_pos = filename.rfind('/');
  return (slash_pos == std::string::npos) ? "" : filename.substr(0, slash_pos);
}

static FILE* fopen_binary_raw(const std::string& filename, const std::string& mode) {
  std::string new_mode = mode;
  if (new_mode.find('b') == std::string::npos) {
    new_mode += 'b';
  }
  FILE* f = fopen(filename.c_str(), new_mode.c_str());
  if (!f) {
    throw cannot_open_file(filename);
  }
  return f;
}

static void fclose_raw(FILE* f) {
  fclose(f);
}

std::unique_ptr<FILE, void (*)(FILE*)> fopen_unique(
    const std::string& filename, const std::string& mode, FILE* dash_file) {
  if (dash_file && (filename == "-")) {
    return std::unique_ptr<FILE, void (*)(FILE*)>(dash_file, +[](FILE*) {});
  }
  return std::unique_ptr<FILE, void (*)(FILE*)>(fopen_binary_raw(filename, mode), fclose_raw);
}

std::shared_ptr<FILE> fopen_shared(const std::string& filename, const std::string& mode, FILE* dash_file) {
  if (dash_file && (filename == "-")) {
    return std::shared_ptr<FILE>(dash_file, [](FILE*) {});
  }
  return std::shared_ptr<FILE>(fopen_binary_raw(filename, mode), fclose_raw);
}

std::string read_all(FILE* f) {
  static const ssize_t read_size = 16 * 1024;

  size_t total_size = 0;
  std::vector<std::string> buffers;
  for (;;) {
    buffers.emplace_back(read_size, 0);
    ssize_t bytes_read = ::fread(buffers.back().data(), 1, read_size, f);
    if (bytes_read < 0) {
      throw io_error(fileno(f));
    }

    total_size += bytes_read;
    if (bytes_read < read_size) {
      buffers.back().resize(bytes_read);
      break;
    }
  }

  if (buffers.size() == 1) {
    return buffers.back();
  }

  std::string ret;
  ret.reserve(total_size);
  for (const std::string& buffer : buffers) {
    ret += buffer;
  }

  return ret;
}

std::string fread(FILE* f, size_t size) {
  std::string data(size, '\0');
  ssize_t ret_size = ::fread(data.data(), 1, size, f);
  if (ret_size < 0) {
    throw io_error(fileno(f));
  } else if (ret_size != static_cast<ssize_t>(size)) {
    data.resize(ret_size);
  }
  return data;
}

void freadx(FILE* f, void* data, size_t size) {
  ssize_t ret_size = ::fread(data, 1, size, f);
  if (ret_size < 0) {
    throw io_error(fileno(f));
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fileno(f), std::format("expected {} bytes, read {} bytes", size, ret_size));
  }
}

void fwritex(FILE* f, const void* data, size_t size) {
  ssize_t ret_size = ::fwrite(data, 1, size, f);
  if (ret_size < 0) {
    throw io_error(fileno(f));
  } else if (ret_size != static_cast<ssize_t>(size)) {
    throw io_error(fileno(f), std::format("expected {} bytes, wrote {} bytes", size, ret_size));
  }
}

std::string freadx(FILE* f, size_t size) {
  std::string ret(size, 0);
  freadx(f, ret.data(), size);
  return ret;
}

void fwritex(FILE* f, const std::string& data) {
  fwritex(f, data.data(), data.size());
}

uint8_t fgetcx(FILE* f) {
  int ret = ::fgetc(f);
  if (ret == EOF) {
    if (feof(f)) {
      throw io_error(fileno(f), "end of stream");
    } else {
      throw io_error(fileno(f), "cannot read from stream");
    }
  }
  return ret;
}

std::string fgets(FILE* f) {
  std::deque<std::string> blocks;
  for (;;) {
    std::string& block = blocks.emplace_back(0x100, '\0');
    if (!::fgets(block.data(), block.size(), f)) {
      blocks.pop_back();
      if (::feof(f)) {
        break;
      } else {
        throw io_error(fileno(f), "cannot read from stream");
      }
    }
    size_t block_bytes = strlen(block.c_str());
    if ((block_bytes < 0x100) || (block[0xFF] == '\n')) {
      block.resize(block_bytes);
      break; // The line ends at the end of this block
    }
  }

  if (blocks.size() == 1) {
    return std::move(blocks.front());
  } else {
    return join(blocks);
  }
}

std::string load_file(const std::string& filename) {
  auto f = fopen_unique(filename, "rb");
  fseek(f.get(), 0, SEEK_END);
  ssize_t file_size = ftell(f.get());
  fseek(f.get(), 0, SEEK_SET);
  return freadx(f.get(), file_size);
}

void save_file(const std::string& filename, const void* data, size_t size) {
  auto f = fopen_unique(filename, "wb");
  fwritex(f.get(), data, size);
}

void save_file(const std::string& filename, const std::string& data) {
  save_file(filename, data.data(), data.size());
}

} // namespace phosg
