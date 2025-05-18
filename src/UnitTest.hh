#pragma once

#include <stdint.h>

#include <format>
#include <functional>
#include <stdexcept>

#include "Strings.hh"

namespace phosg {

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
void expect_raises_fn(const char* file, uint64_t line, std::function<void()> fn) {
  try {
    fn();
    expect_generic(false, "expected exception, but none raised", file, line);
  } catch (const ExcT&) {
    return;
  } catch (const std::exception& e) {
    std::string msg = std::format("incorrect exception type raised (what: {})", e.what());
    expect_generic(false, msg.c_str(), file, line);
  } catch (...) {
    // TODO: It'd be nice to show SOMETHING about the thrown exception here,
    // but it's probably pretty rare to catch something that isn't a
    // std::exception anyway.
    expect_generic(false, "incorrect exception type raised", file, line);
  }
}

template <>
void expect_raises_fn<std::exception>(const char* file, uint64_t line, std::function<void()> fn);

#define expect_raises(type, fn) expect_raises_fn<type>(__FILE__, __LINE__, fn)

} // namespace phosg
