#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
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
