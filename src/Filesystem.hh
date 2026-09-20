#pragma once

#include "Platform.hh"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <dirent.h>
#ifndef PHOSG_WINDOWS
#include <poll.h>
#include <sys/uio.h>
#endif

#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef PHOSG_WINDOWS
#include "Filesystem-Unix.hh"
#endif

namespace phosg {

class cannot_open_file : virtual public std::runtime_error {
public:
  cannot_open_file(int fd);
  cannot_open_file(std::string_view filename);

  int error;
};

class io_error : virtual public std::runtime_error {
public:
  io_error(int fd);
  io_error(int fd, std::string_view what);

  int error;
};

std::string_view basename(std::string_view filename);
std::string_view dirname(std::string_view filename);

std::unique_ptr<FILE, void (*)(FILE*)> fopen_unique(
    std::string_view filename, std::string_view mode = "rb", FILE* dash_file = nullptr);
std::shared_ptr<FILE> fopen_shared(std::string_view filename, std::string_view mode = "rb", FILE* dash_file = nullptr);

std::string read_all(FILE* f);
std::string fread(FILE* f, size_t size);
void freadx(FILE* f, void* data, size_t size);
void fwritex(FILE* f, const void* data, size_t size);
std::string freadx(FILE* f, size_t size);
void fwritex(FILE* f, const std::string& data);
void fwritex(FILE* f, std::string_view data);
uint8_t fgetcx(FILE* f);

std::string fgets(FILE* f);

template <typename T>
T freadx(FILE* f) {
  T t;
  freadx(f, &t, sizeof(T));
  return t;
}

template <typename T>
void fwritex(FILE* f, const T& t) {
  fwritex(f, &t, sizeof(T));
}

std::string load_file(std::string_view filename);
void save_file(std::string_view filename, const void* data, size_t size);
void save_file(std::string_view filename, std::string_view data);

template <typename T>
T load_object_file(std::string_view filename, bool allow_oversize = false) {
  auto f = fopen_unique(filename, "rb");
  T ret;
  freadx(f.get(), &ret, sizeof(ret));
  if (!allow_oversize) {
    std::string extra = fread(f.get(), 1);
    if (!extra.empty()) {
      throw std::runtime_error(std::format("file {} is too large", filename));
    }
  }
  return ret;
}

template <typename T>
void save_object_file(std::string_view filename, const T& obj) {
  save_file(filename, &obj, sizeof(obj));
}

template <typename T>
std::vector<T> load_vector_file(std::string_view filename) {
  auto f = fopen_unique(filename, "rb");
  fseek(f.get(), 0, SEEK_END);
  size_t file_size = ftell(f.get());
  fseek(f.get(), 0, SEEK_SET);
  size_t item_count = file_size / sizeof(T);
  size_t read_size = item_count * sizeof(T);

  std::vector<T> ret(item_count);
  freadx(f.get(), ret.data(), read_size);
  return ret;
}

template <typename T>
void save_vector_file(std::string_view filename, const std::vector<T>& v) {
  save_file(filename, v.data(), v.size() * sizeof(T));
}

} // namespace phosg
