#include "Random.hh"

#include <string.h>
#ifdef WINDOWS
#include <windows.h>
#include <wincrypt.h>
#endif

#include <stdexcept>
#include <string>

#include "Filesystem.hh"

using namespace std;


void random_data(void* data, size_t bytes) {
#ifdef WINDOWS
  static thread_local bool crypt_prov_valid = false;
  static thread_local HCRYPTPROV crypt_prov;

  if (!crypt_prov_valid) {
    if (!CryptAcquireContext(&crypt_prov, nullptr, "Microsoft Base Cryptographic Provider v1.0", PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
      throw runtime_error("can\'t acquire crypt context");
    }
    crypt_prov_valid = true;
  }

  if (!CryptGenRandom(crypt_prov, bytes, static_cast<uint8_t*>(data))) {
    throw runtime_error("can\'t generate random data");
  }

#else
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
