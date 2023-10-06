#include <sys/time.h>
#include <unistd.h>

#include "Time.hh"
#include "UnitTest.hh"

using namespace std;

int main(int, char**) {

  fprintf(stderr, "-- usleep\n");
  uint64_t start_time = now();
  expect_ge(now(), start_time);
  usleep(1000000);
  expect_ge(now(), start_time + 1000000);

  fprintf(stderr, "-- format_time\n");
  expect_eq(format_time(static_cast<uint64_t>(0)), "1970-01-01 00:00:00");
  expect_eq(format_time(1478915749908529), "2016-11-12 01:55:49");

  {
    fprintf(stderr, "-- format_duration\n");
    expect_eq("0.00000", format_duration(0));
    expect_eq("0.22222", format_duration(222222));
    expect_eq("2.22222", format_duration(2222222));
    expect_eq("12.2222", format_duration(12222222));
    expect_eq("1:02.22", format_duration(62222222));
    expect_eq("1:10.00", format_duration(69999999));
    expect_eq("1:12.22", format_duration(72222222));
    expect_eq("11:12.2", format_duration(672222222));
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

  printf("TimeTest: all tests passed\n");
  return 0;
}
