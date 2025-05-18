#include "UnitTest.hh"

#include <inttypes.h>

#include <format>

#include "Strings.hh"

using namespace std;

namespace phosg {

expectation_failed::expectation_failed(const char* msg, const char* file, uint64_t line)
    : logic_error(std::format("failure at {}:{}: {}", file, line, msg)),
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
void expect_raises_fn<std::exception>(const char* file, uint64_t line, std::function<void()> fn) {
  try {
    fn();
    expect_generic(false, "expected exception, but none raised", file, line);
  } catch (const std::exception& e) {
    return;
  } catch (...) {
    // TODO: It'd be nice to show SOMETHING about the thrown exception here,
    // but it's probably pretty rare to catch something that isn't a
    // std::exception anyway.
    expect_generic(false, "incorrect exception type raised", file, line);
  }
};

} // namespace phosg
