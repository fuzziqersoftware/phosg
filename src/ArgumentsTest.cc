#include <inttypes.h>
#include <stdio.h>

#include "Arguments.hh"
#include "Encoding.hh"
#include "UnitTest.hh"

int main(int, char**) {
  Arguments a("pos0 --named1 300 --named2=value2 4.0 --int3=40000 --float4=2.0");

  expect_raises<invalid_argument>([&]() {
    a.assert_none_unused();
  });

  // Positional arguments
  expect_eq("pos0", a.get<string>(0));
  expect_eq("300", a.get<string>(1));
  expect_eq(300, a.get<int16_t>(1));
  expect_eq(300, a.get<uint16_t>(1));
  expect_raises<invalid_argument>([&]() {
    a.get<int8_t>(1);
  });
  expect_raises<invalid_argument>([&]() {
    a.get<uint8_t>(1);
  });
  expect_eq(4.0f, a.get<float>(2));
  expect_eq(4.0, a.get<double>(2));
  expect_raises<out_of_range>([&]() {
    a.get<string>(3);
  });

  expect_raises<invalid_argument>([&]() {
    a.assert_none_unused();
  });

  // Named arguments
  expect_raises<out_of_range>([&]() {
    a.get<int16_t>("missing");
  });
  expect_eq(true, a.get<bool>("named1"));
  expect_eq(false, a.get<bool>("missing"));
  expect_eq("value2", a.get<string>("named2"));
  expect_eq(40000, a.get<int32_t>("int3"));
  expect_eq(40000, a.get<uint16_t>("int3"));
  expect_raises<invalid_argument>([&]() {
    a.get<int16_t>("int3");
  });
  expect_raises<invalid_argument>([&]() {
    a.get<uint8_t>("int3");
  });
  expect_eq(2.0f, a.get<float>("float4"));
  expect_eq(2.0, a.get<double>("float4"));

  a.assert_none_unused();

  printf("ArgumentTest: all tests passed\n");
  return 0;
}
