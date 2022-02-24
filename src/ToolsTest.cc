#include <stdexcept>

#include "Tools.hh"
#include "UnitTest.hh"

using namespace std;



int main(int, char** argv) {
  bool called = false;
  {
    auto g1 = on_close_scope([&]() {
      called = true;
    });
    expect(!called);
  }
  expect(called);

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
