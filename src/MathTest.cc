#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "Math.hh"
#include "UnitTest.hh"

using namespace std;
using namespace phosg;

int main(int, char**) {
  {
    fprintf(stderr, "-- gcd\n");
    expect_eq(30, gcd(30, 30));
    expect_eq(10, gcd(30, 80));
    expect_eq(1, gcd(30, 1));
  }

  {
    fprintf(stderr, "-- reduce_fraction\n");
    expect_eq(make_pair(1, 2), reduce_fraction(4, 8));
    expect_eq(make_pair(1, 1), reduce_fraction(4, 4));
    expect_eq(make_pair(1, 1), reduce_fraction(1, 1));
    expect_eq(make_pair(15, 2), reduce_fraction(45, 6));
  }

  return 0;
}
