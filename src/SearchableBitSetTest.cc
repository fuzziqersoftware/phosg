#include <stdio.h>

#include <string>

#include "SearchableBitSet.hh"
#include "UnitTest.hh"

template <typename T>
void test_base(T bs) {
  expect_eq(bs.num_levels(), 1);

  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), false);
  }
  bs.set(17);
  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), (z == 17));
  }
  bs.clear(16);
  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), (z == 17));
  }
  bs.clear(17);
  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), false);
  }

  bs.set(10);
  bs.set(15);
  bs.set(20);
  expect_eq(bs.find_set_at_or_after(0), 10);
  expect_eq(bs.find_set_at_or_after(10), 10);
  expect_eq(bs.find_set_at_or_after(11), 15);
  expect_eq(bs.find_set_at_or_after(15), 15);
  expect_eq(bs.find_set_at_or_after(16), 20);
  expect_eq(bs.find_set_at_or_after(20), 20);
  expect_eq(bs.find_set_at_or_after(21), bs.count());
  expect_eq(bs.find_set_at_or_after(31), bs.count());
  expect_eq(bs.find_set_at_or_before(0), bs.count());
  expect_eq(bs.find_set_at_or_before(9), bs.count());
  expect_eq(bs.find_set_at_or_before(10), 10);
  expect_eq(bs.find_set_at_or_before(14), 10);
  expect_eq(bs.find_set_at_or_before(15), 15);
  expect_eq(bs.find_set_at_or_before(19), 15);
  expect_eq(bs.find_set_at_or_before(20), 20);
  expect_eq(bs.find_set_at_or_before(31), 20);
}

template <typename T>
void test_single_index(T bs) {
  expect_eq(bs.num_levels(), 2);

  for (size_t z = 0; z < 0x1000; z++) {
    expect_eq(bs.check(z), false);
  }
  bs.set(0x3BD);
  for (size_t z = 0; z < 0x1000; z++) {
    expect_eq(bs.check(z), (z == 0x3BD));
  }
  bs.clear(0x53A);
  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), (z == 0x3BD));
  }
  bs.clear(0x3BD);
  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), false);
  }

  bs.set(0x200);
  bs.set(0x500);
  bs.set(0x505);
  bs.set(0x53F);
  bs.set(0x800);
  expect_eq(bs.find_set_at_or_after(0x000), 0x200);
  expect_eq(bs.find_set_at_or_after(0x200), 0x200);
  expect_eq(bs.find_set_at_or_after(0x201), 0x500);
  expect_eq(bs.find_set_at_or_after(0x500), 0x500);
  expect_eq(bs.find_set_at_or_after(0x501), 0x505);
  expect_eq(bs.find_set_at_or_after(0x505), 0x505);
  expect_eq(bs.find_set_at_or_after(0x506), 0x53F);
  expect_eq(bs.find_set_at_or_after(0x53F), 0x53F);
  expect_eq(bs.find_set_at_or_after(0x540), 0x800);
  expect_eq(bs.find_set_at_or_after(0x800), 0x800);
  expect_eq(bs.find_set_at_or_after(0x801), bs.count());
  expect_eq(bs.find_set_at_or_after(0xFFF), bs.count());
  expect_eq(bs.find_set_at_or_before(0x000), bs.count());
  expect_eq(bs.find_set_at_or_before(0x1FF), bs.count());
  expect_eq(bs.find_set_at_or_before(0x200), 0x200);
  expect_eq(bs.find_set_at_or_before(0x4FF), 0x200);
  expect_eq(bs.find_set_at_or_before(0x500), 0x500);
  expect_eq(bs.find_set_at_or_before(0x504), 0x500);
  expect_eq(bs.find_set_at_or_before(0x505), 0x505);
  expect_eq(bs.find_set_at_or_before(0x53E), 0x505);
  expect_eq(bs.find_set_at_or_before(0x53F), 0x53F);
  expect_eq(bs.find_set_at_or_before(0x7FF), 0x53F);
  expect_eq(bs.find_set_at_or_before(0x800), 0x800);
  expect_eq(bs.find_set_at_or_before(0xFFF), 0x800);
}

template <typename T>
void test_double_index(T bs) {
  expect_eq(bs.num_levels(), 3);

  for (size_t z = 0; z < 0x40000; z++) {
    expect_eq(bs.check(z), false);
  }
  bs.set(0x3BD0);
  for (size_t z = 0; z < 0x40000; z++) {
    expect_eq(bs.check(z), (z == 0x3BD0));
  }
  bs.clear(0x53A0);
  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), (z == 0x3BD0));
  }
  bs.clear(0x3BD0);
  for (size_t z = 0; z < 32; z++) {
    expect_eq(bs.check(z), false);
  }

  bs.set(0x2000);
  bs.set(0x5000);
  bs.set(0x5005);
  bs.set(0x503F);
  bs.set(0x8000);
  expect_eq(bs.find_set_at_or_after(0x0000), 0x2000);
  expect_eq(bs.find_set_at_or_after(0x2000), 0x2000);
  expect_eq(bs.find_set_at_or_after(0x2001), 0x5000);
  expect_eq(bs.find_set_at_or_after(0x5000), 0x5000);
  expect_eq(bs.find_set_at_or_after(0x5001), 0x5005);
  expect_eq(bs.find_set_at_or_after(0x5005), 0x5005);
  expect_eq(bs.find_set_at_or_after(0x5006), 0x503F);
  expect_eq(bs.find_set_at_or_after(0x503F), 0x503F);
  expect_eq(bs.find_set_at_or_after(0x5040), 0x8000);
  expect_eq(bs.find_set_at_or_after(0x8000), 0x8000);
  expect_eq(bs.find_set_at_or_after(0x8001), bs.count());
  expect_eq(bs.find_set_at_or_after(0x3FFFF), bs.count());
  expect_eq(bs.find_set_at_or_before(0x0000), bs.count());
  expect_eq(bs.find_set_at_or_before(0x1FFF), bs.count());
  expect_eq(bs.find_set_at_or_before(0x2000), 0x2000);
  expect_eq(bs.find_set_at_or_before(0x4FFF), 0x2000);
  expect_eq(bs.find_set_at_or_before(0x5000), 0x5000);
  expect_eq(bs.find_set_at_or_before(0x5004), 0x5000);
  expect_eq(bs.find_set_at_or_before(0x5005), 0x5005);
  expect_eq(bs.find_set_at_or_before(0x503E), 0x5005);
  expect_eq(bs.find_set_at_or_before(0x503F), 0x503F);
  expect_eq(bs.find_set_at_or_before(0x7FFF), 0x503F);
  expect_eq(bs.find_set_at_or_before(0x8000), 0x8000);
  expect_eq(bs.find_set_at_or_before(0x3FFFF), 0x8000);
}

int main(int, char**) {
  phosg::fwrite_fmt(stdout, "-- SearchableBitSet<32> (no index)\n");
  test_base(phosg::SearchableBitSet<32>());
  phosg::fwrite_fmt(stdout, "-- SearchableBitVector(32) (no index)\n");
  test_base(phosg::SearchableBitVector(32));

  phosg::fwrite_fmt(stdout, "-- SearchableBitSet<0x1000> (1 index level)\n");
  test_single_index(phosg::SearchableBitSet<0x1000>());
  phosg::fwrite_fmt(stdout, "-- SearchableBitVector(0x1000) (1 index level)\n");
  test_single_index(phosg::SearchableBitVector(0x1000));

  phosg::fwrite_fmt(stdout, "-- SearchableBitSet<0x40000> (2 index levels)\n");
  test_double_index(phosg::SearchableBitSet<0x40000>());
  phosg::fwrite_fmt(stdout, "-- SearchableBitVector(0x40000) (2 index levels)\n");
  test_double_index(phosg::SearchableBitVector(0x40000));

  phosg::fwrite_fmt(stdout, "SearchableBitSet: all tests passed\n");
  return 0;
}
