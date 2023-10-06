#include <stdexcept>

#include "Tools.hh"
#include "UnitTest.hh"

using namespace std;

int main(int, char**) {
  printf("-- on_close_scope\n");
  bool called = false;
  {
    auto g1 = on_close_scope([&]() {
      called = true;
    });
    expect(!called);
  }
  expect(called);

  const size_t num_threads = thread::hardware_concurrency();

  printf("-- parallel_range\n");
  vector<uint8_t> hits(0x1000000, 0);
  auto handle_value = [&](uint64_t v, size_t thread_num) -> bool {
    hits[v] = thread_num + 1;
    return false;
  };
  parallel_range<uint64_t>(handle_value, 0, hits.size(), num_threads, nullptr);

  vector<size_t> thread_counts(num_threads, 0);
  for (size_t x = 0; x < hits.size(); x++) {
    expect_ne(hits[x], 0);
    thread_counts.at(hits[x] - 1)++;
  }

  size_t sum = 0;
  for (size_t x = 0; x < thread_counts.size(); x++) {
    expect_ne(thread_counts[x], 0);
    fprintf(stderr, "---- thread %zu: %zu\n", x, thread_counts[x]);
    sum += thread_counts[x];
  }
  expect_eq(sum, hits.size());

  printf("-- parallel_range return value\n");
  uint64_t target_value = 0xC349;
  auto is_equal = [&](uint64_t v, size_t) -> bool {
    return (v == target_value);
  };
  expect_eq((parallel_range<uint64_t>(is_equal, 0, 0x10000, num_threads, nullptr)), target_value);
  // Note: We can't check that parallel_range ends early when fn returns true
  // because it's not actually guaranteed to do so - it's only guaranteed to
  // return any of the values for which fn returns true. One could imagine a sequence of events in
  // which the target value's call takes a very long time, and all other threads
  // could finish checking all other values before the target one returns true.
  target_value = 0xCC349; // > end_value; should not be found
  expect_eq((parallel_range<uint64_t>(is_equal, 0, 0x10000, num_threads, nullptr)), 0x10000);

  printf("ToolsTest: all tests passed\n");
  return 0;
}
