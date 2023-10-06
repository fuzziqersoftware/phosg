#include <stdexcept>

#include "UnitTest.hh"

using namespace std;

#define expect_fails(x)                                                       \
  do {                                                                        \
    try {                                                                     \
      x;                                                                      \
      expect_msg(false, #x " did not throw");                                 \
    } catch (const expectation_failed& e) {                                   \
    } catch (...) {                                                           \
      expect_msg(false, #x " threw something that isn't expectation_failed"); \
    }                                                                         \
  } while (false);

int main(int, char**) {

  // make sure expect_msg works, since we depend on it for expect_fails later
  expect_msg(true, "omg wut");
  try {
    expect_msg(false, "omg wut");
    throw logic_error("expect_msg(false, ...) didn\'t throw");
  } catch (const expectation_failed& e) {
  }

  expect_eq(0, 0);
  expect_ne(1, 0);
  expect_gt(3, 2);
  expect_ge(3, 3);
  expect_ge(3, 2);
  expect_lt(2, 3);
  expect_le(3, 3);
  expect_le(2, 3);
  expect(true);

  expect_fails(expect_eq(0, -1));
  expect_fails(expect_ne(5, 5));
  expect_fails(expect_gt(1, 2));
  expect_fails(expect_gt(2, 2));
  expect_fails(expect_ge(2, 3));
  expect_fails(expect_lt(2, 1));
  expect_fails(expect_lt(2, 2));
  expect_fails(expect_le(4, 3));
  expect_fails(expect(false));

  expect_fails(expect_raises<runtime_error>([&]() {
    return;
  }));

  expect_fails(expect_raises<runtime_error>([&]() {
    throw logic_error("omg hax");
  }));

  expect_raises<runtime_error>([&]() {
    throw runtime_error("omg hax");
  });

  expect_raises<exception>([&]() {
    throw runtime_error("omg hax");
  });

  printf("UnitTestTest: all tests passed\n");
  return 0;
}
