#pragma once

#include <stdint.h>

#include <stdexcept>

class expectation_failed : public std::logic_error {
public:
  expectation_failed(const char* msg, const char* file, uint64_t line);

  const char* msg;
  const char* file;
  uint64_t line;
};

#define expect_eq(a, b) expect_msg((a) == (b), #a " != " #b)
#define expect_ne(a, b) expect_msg((a) != (b), #a " == " #b)
#define expect_gt(a, b) expect_msg((a) >  (b), #a " <= " #b)
#define expect_ge(a, b) expect_msg((a) >= (b), #a " < " #b)
#define expect_lt(a, b) expect_msg((a) <  (b), #a " >= " #b)
#define expect_le(a, b) expect_msg((a) <= (b), #a " > " #b)
#define expect(pred) expect_msg((pred), "!(" #pred ")")

#define expect_msg(pred, msg) expect_generic((pred), (msg), __FILE__, __LINE__)

void expect_generic(bool pred, const char* msg, const char* file, uint64_t line);
