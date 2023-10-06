#include <stdio.h>

#include <string>

#include "LRUSet.hh"
#include "UnitTest.hh"

using namespace std;

int main(int, char**) {
  LRUSet<string> c;

  expect_eq(c.size(), 0);
  expect_eq(c.count(), 0);

  expect(c.insert("key1", 40));
  expect_eq(c.size(), 40);
  expect_eq(c.count(), 1);

  expect(c.emplace("key2", 80));
  expect_eq(c.size(), 120);
  expect_eq(c.count(), 2);

  expect(c.emplace("key3", 80));
  expect_eq(c.size(), 200);
  expect_eq(c.count(), 3);

  expect(c.change_size("key1", 80));
  expect_eq(c.size(), 240);
  expect_eq(c.count(), 3);

  expect_eq(c.emplace("key2", 100), false);
  expect_eq(c.size(), 260);
  expect_eq(c.count(), 3);

  expect_eq(c.change_size("key4", 300), false);
  expect_eq(c.size(), 260);
  expect_eq(c.count(), 3);

  expect_eq(c.touch("key4", 300), false);
  expect_eq(c.size(), 260);
  expect_eq(c.count(), 3);

  expect(c.touch("key1", 100));
  expect_eq(c.size(), 280);
  expect_eq(c.count(), 3);

  LRUSet<string> d;
  expect_eq(d.size(), 0);
  expect_eq(d.count(), 0);

  d.swap(c);
  expect_eq(c.size(), 0);
  expect_eq(c.count(), 0);
  expect_eq(d.size(), 280);
  expect_eq(d.count(), 3);

  auto last = d.peek();
  expect_eq(last.first, "key3");
  expect_eq(last.second, 80);
  expect_eq(d.size(), 280);
  expect_eq(d.count(), 3);

  auto evicted = d.evict_object();
  expect_eq(evicted.first, "key3");
  expect_eq(evicted.second, 80);
  expect_eq(d.size(), 200);
  expect_eq(d.count(), 2);

  last = d.peek();
  expect_eq(last.first, "key2");
  expect_eq(last.second, 100);
  expect_eq(d.size(), 200);
  expect_eq(d.count(), 2);

  evicted = d.evict_object();
  expect_eq(evicted.first, "key2");
  expect_eq(evicted.second, 100);
  expect_eq(d.size(), 100);
  expect_eq(d.count(), 1);

  last = d.peek();
  expect_eq(last.first, "key1");
  expect_eq(last.second, 100);
  expect_eq(d.size(), 100);
  expect_eq(d.count(), 1);

  evicted = d.evict_object();
  expect_eq(evicted.first, "key1");
  expect_eq(evicted.second, 100);
  expect_eq(d.size(), 0);
  expect_eq(d.count(), 0);

  printf("LRUSetTest: all tests passed\n");

  return 0;
}
