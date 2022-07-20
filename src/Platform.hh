#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__CYGWIN__)
#define PHOSG_WINDOWS

#elif __APPLE__
#define PHOSG_MACOS

#elif __linux__
#define PHOSG_LINUX

#elif __unix__
#define PHOSG_UNIX

#elif defined(_POSIX_VERSION)
#define PHOSG_POSIX

#else
#error "Unknown platform"
#endif



// Try to determine endianess from GCC defines first. If they aren't available,
// use some constants to try to figure it out
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define PHOSG_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define PHOSG_BIG_ENDIAN

#else
  #define PHOSG_LITTLE_ENDIAN_VALUE 0x31323334UL
  #define PHOSG_BIG_ENDIAN_VALUE    0x34333231UL
  #define PHOSG_ENDIAN_ORDER_VALUE  ('1234')

  #if PHOSG_ENDIAN_ORDER_VALUE == PHOSG_LITTLE_ENDIAN_VALUE
    #define PHOSG_LITTLE_ENDIAN
  #elif PHOSG_ENDIAN_ORDER_VALUE == PHOSG_BIG_ENDIAN_VALUE
    #define PHOSG_BIG_ENDIAN
  #else
    #error "Unrecognized host system endianness"
  #endif
#endif
