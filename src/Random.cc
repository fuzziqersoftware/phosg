#include "Random.hh"

#include "Platform.hh"

#include <string.h>

#ifdef PHOSG_WINDOWS
#include <windows.h>

#include <bcrypt.h>
#include <ntstatus.h>
#endif

#include <format>
#include <stdexcept>
#include <string>

#include "Filesystem.hh"

using namespace std;

namespace phosg {

string random_data(size_t bytes) {
  string ret(bytes, '\0');
  random_data(ret.data(), ret.size());
  return ret;
}

void random_data(void* data, size_t bytes) {
#ifndef PHOSG_WINDOWS
  static scoped_fd fd("/dev/urandom", O_RDONLY);
  static thread_local string buffer;

  while (buffer.size() < bytes) {
    memcpy(data, buffer.data(), buffer.size());
    bytes -= buffer.size();
    data = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(data) + buffer.size());
    buffer = readx(fd, 4096);
  }

  if (bytes) {
    memcpy(data, buffer.data() + buffer.size() - bytes, bytes);
    buffer.resize(buffer.size() - bytes);
  }
#else
  auto status = BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(data), bytes, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (status != STATUS_SUCCESS) {
    throw std::runtime_error(std::format("unable to generate random data: {:08X}", status));
  }
#endif
}

int64_t random_int(int64_t low, int64_t high) {
  int64_t range = high - low + 1;
  if (range > 0xFFFFFFFF) {
    return low + (random_object<uint64_t>() % range);
  } else if (range > 0xFFFF) {
    return low + (random_object<uint32_t>() % range);
  } else if (range > 0xFF) {
    return low + (random_object<uint16_t>() % range);
  } else {
    return low + (random_object<uint8_t>() % range);
  }
}

} // namespace phosg
