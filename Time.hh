#pragma once

#include <stdint.h>

#include <string>


uint64_t now();
std::string format_time(uint64_t t);

struct timeval usecs_to_timeval(uint64_t usecs);
