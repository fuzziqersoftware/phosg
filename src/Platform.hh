#pragma once

#include <stdint.h>

namespace phosg {

enum class Platform {
  MACOS = 0,
  LINUX,
  WINDOWS,
  UNIX,
  POSIX,
};

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__CYGWIN__)
#define PHOSG_WINDOWS
constexpr Platform PLATFORM = Platform::WINDOWS;

#elif __APPLE__
#define PHOSG_MACOS
constexpr Platform PLATFORM = Platform::MACOS;

#elif __linux__
#define PHOSG_LINUX
constexpr Platform PLATFORM = Platform::LINUX;

#elif __unix__
#define PHOSG_UNIX
constexpr Platform PLATFORM = Platform::UNIX;

#elif defined(_POSIX_VERSION)
#define PHOSG_POSIX
constexpr Platform PLATFORM = Platform::POSIX;

#else
#error "Unknown platform"
#endif

constexpr inline bool is_windows() {
  return PLATFORM == Platform::WINDOWS;
}
constexpr inline bool is_macos() {
  return PLATFORM == Platform::MACOS;
}

// Try to determine endianess from GCC defines first. If they aren't available,
// use some constants to try to figure it out
// clang-format off
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  #define PHOSG_LITTLE_ENDIAN
  static constexpr bool IS_BIG_ENDIAN = false;
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  #define PHOSG_BIG_ENDIAN
  static constexpr bool IS_BIG_ENDIAN = true;

#else
  #define PHOSG_LITTLE_ENDIAN_VALUE 0x31323334UL
  #define PHOSG_BIG_ENDIAN_VALUE    0x34333231UL
  #define PHOSG_ENDIAN_ORDER_VALUE  ('1234')

  #if PHOSG_ENDIAN_ORDER_VALUE == PHOSG_LITTLE_ENDIAN_VALUE
    #define PHOSG_LITTLE_ENDIAN
    static constexpr bool IS_BIG_ENDIAN = false;
  #elif PHOSG_ENDIAN_ORDER_VALUE == PHOSG_BIG_ENDIAN_VALUE
    #define PHOSG_BIG_ENDIAN
    static constexpr bool IS_BIG_ENDIAN = true;
  #else
    #error "Unrecognized host system endianness"
  #endif
#endif
// clang-format on

#if (SIZE_MAX == 0xFF)
#define SIZE_T_BITS 8
#elif (SIZE_MAX == 0xFFFF)
#define SIZE_T_BITS 16
#elif (SIZE_MAX == 0xFFFFFFFF)
#define SIZE_T_BITS 32
#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
#define SIZE_T_BITS 64
#else
#error SIZE_MAX is not a recognized value
#endif

} // namespace phosg
