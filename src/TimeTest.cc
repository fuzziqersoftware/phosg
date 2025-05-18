#include <sys/time.h>
#include <unistd.h>

#include "Time.hh"
#include "UnitTest.hh"

using namespace std;
using namespace phosg;

int main(int, char**) {
  fwrite_fmt(stderr, "-- usleep\n");
  uint64_t start_time = now();
  expect_ge(now(), start_time);
  usleep(1000000);
  expect_ge(now(), start_time + 1000000);

  fwrite_fmt(stderr, "-- format_time\n");
  expect_eq(format_time(static_cast<uint64_t>(0)), "1970-01-01 00:00:00.000000");
  expect_eq(format_time(1478915749908529), "2016-11-12 01:55:49.908529");
  expect_eq(format_time(1478915749000529), "2016-11-12 01:55:49.000529");
  expect_eq(format_time(1478915749908000), "2016-11-12 01:55:49.908000");

  {
    fwrite_fmt(stderr, "-- format_duration\n");
    expect_eq("0.000000", format_duration(0));
    expect_eq("0.222222", format_duration(222222));
    expect_eq("2.222222", format_duration(2222222));
    expect_eq("12.222222", format_duration(12222222));
    expect_eq("1:02.222", format_duration(62222222));
    expect_eq("1:10.000", format_duration(69999999));
    expect_eq("1:12.222", format_duration(72222222));
    expect_eq("11:12.222", format_duration(672222222));
    expect_eq("1:01:12", format_duration(3672222222));
    expect_eq("1:11:12", format_duration(4272222222));
    expect_eq("11:11:12", format_duration(40272222222));
    expect_eq("5:11:11:12", format_duration(472272222222));

    expect_eq("0", format_duration(438294, 0));
    expect_eq("0.4", format_duration(438294, 1));
    expect_eq("0.44", format_duration(438294, 2));
    expect_eq("0.438", format_duration(438294, 3));
    expect_eq("0.4383", format_duration(438294, 4));
    expect_eq("0.43829", format_duration(438294, 5));
    expect_eq("0.438294", format_duration(438294, 6));
  }

  fwrite_fmt(stdout, "TimeTest: all tests passed\n");
  return 0;
}
