#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#include "Time.hh"

using namespace std;


int main(int argc, char** argv) {

  uint64_t start_time = now();
  assert(now() >= start_time);
  usleep(1000000);
  assert(now() >= start_time + 1000000);

  assert(format_time(0) == "1970-01-01 00:00:00");
  assert(format_time(1478915749908529) == "2016-11-12 01:55:49");

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
