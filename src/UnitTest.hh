#pragma once

#include <stdint.h>

#include <functional>
#include <stdexcept>

#include "Strings.hh"

class expectation_failed : public std::logic_error {
public:
  expectation_failed(const char* msg, const char* file, uint64_t line);

  const char* msg;
  const char* file;
  uint64_t line;
};

#define expect_eq(a, b) expect_msg((a) == (b), #a " != " #b)
#define expect_ne(a, b) expect_msg((a) != (b), #a " == " #b)
#define expect_gt(a, b) expect_msg((a) > (b), #a " <= " #b)
#define expect_ge(a, b) expect_msg((a) >= (b), #a " < " #b)
#define expect_lt(a, b) expect_msg((a) < (b), #a " >= " #b)
#define expect_le(a, b) expect_msg((a) <= (b), #a " > " #b)
#define expect(pred) expect_msg((pred), "!(" #pred ")")

#define expect_msg(pred, msg) expect_generic((pred), (msg), __FILE__, __LINE__)

void expect_generic(bool pred, const char* msg, const char* file, uint64_t line);

template <typename ExcT>
void expect_raises(std::function<void()> fn) {
  try {
    fn();
    expect_msg(false, "expected exception, but none raised");
  } catch (const ExcT&) {
    return;
  } catch (const std::exception& e) {
    std::string msg = string_printf("incorrect exception type raised (what: %s)", e.what());
    expect_msg(false, msg.c_str());
  } catch (...) {
    // TODO: It'd be nice to show SOMETHING about the thrown exception here,
    // but it's probably pretty rare to catch something that isn't a
    // std::exception anyway.
    expect_msg(false, "incorrect exception type raised");
  }
};

template <>
void expect_raises<std::exception>(std::function<void()> fn);
