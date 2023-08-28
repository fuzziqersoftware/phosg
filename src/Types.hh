#pragma once

#include <stdexcept>

template <typename...>
struct always_false {
  static constexpr bool v = false;
};

template <typename T>
T enum_for_name(const char*) {
  static_assert(always_false<T>::v, "unspecialized enum_for_name should never be called");
  throw std::logic_error("unspecialized enum_for_name should never be called");
}

template <typename T>
const char* name_for_enum(T) {
  static_assert(always_false<T>::v, "unspecialized name_for_enum should never be called");
  throw std::logic_error("unspecialized name_for_enum should never be called");
}
