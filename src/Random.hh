#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>

std::string random_data(size_t bytes);
void random_data(void* data, size_t bytes);

template <typename T>
T random_object() {
  T ret;
  random_data(&ret, sizeof(T));
  return ret;
}

int64_t random_int(int64_t low, int64_t high);
