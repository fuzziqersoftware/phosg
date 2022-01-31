#include "UnitTest.hh"

#include <inttypes.h>

#include "Strings.hh"

using namespace std;


expectation_failed::expectation_failed(const char* msg, const char* file,
    uint64_t line) : logic_error(string_printf("failure at %s:%" PRIu64 ": %s",
      file, line, msg)), msg(msg), file(file), line(line) { }

void expect_generic(bool pred, const char* msg, const char* file, uint64_t line) {
  if (pred) {
    return;
  }

  throw expectation_failed(msg, file, line);
}
