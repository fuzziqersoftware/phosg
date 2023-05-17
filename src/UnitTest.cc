#include "UnitTest.hh"

#include <inttypes.h>

#include "Strings.hh"

using namespace std;

expectation_failed::expectation_failed(const char* msg, const char* file, uint64_t line)
    : logic_error(string_printf("failure at %s:%" PRIu64 ": %s", file, line, msg)),
      msg(msg),
      file(file),
      line(line) {}

void expect_generic(bool pred, const char* msg, const char* file, uint64_t line) {
  if (pred) {
    return;
  }

  throw expectation_failed(msg, file, line);
}

template <>
void expect_raises<std::exception>(std::function<void()> fn) {
  try {
    fn();
    expect_msg(false, "expected exception, but none raised");
  } catch (const std::exception& e) {
    return;
  } catch (...) {
    // TODO: It'd be nice to show SOMETHING about the thrown exception here,
    // but it's probably pretty rare to catch something that isn't a
    // std::exception anyway.
    expect_msg(false, "incorrect exception type raised");
  }
};
