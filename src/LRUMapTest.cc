#include <stdio.h>

#include <string>

#include "LRUMap.hh"
#include "UnitTest.hh"

using namespace std;

int main(int, char**) {
  LRUMap<string, string> c;

  expect_eq(c.size(), 0);
  expect_eq(c.count(), 0);

  expect_raises<out_of_range>([&]() {
    c.at("key1");
  });

  expect(c.insert("key1", "value0", 30));
  expect_eq(c.size(), 30);
  expect_eq(c.count(), 1);
  expect_eq(c.at("key1"), "value0");

  expect(!c.insert("key1", "value1", 40));
  expect_eq(c.size(), 40);
  expect_eq(c.count(), 1);
  expect_eq(c.at("key1"), "value1");

  expect(c.emplace("key2", "value2", 80));
  expect_eq(c.size(), 120);
  expect_eq(c.count(), 2);
  expect_eq(c.at("key1"), "value1");
  expect_eq(c.at("key2"), "value2");

  expect(c.emplace("key3", "value3", 80));
  expect_eq(c.size(), 200);
  expect_eq(c.count(), 3);
  expect_eq(c.at("key1"), "value1");
  expect_eq(c.at("key2"), "value2");
  expect_eq(c.at("key3"), "value3");

  expect(c.change_size("key1", 80));
  expect_eq(c.size(), 240);
  expect_eq(c.count(), 3);

  expect(c.change_size("key3", 80, false));
  expect_eq(c.size(), 240);
  expect_eq(c.count(), 3);

  expect_eq(c.emplace("key2", "value5", 100), false);
  expect_eq(c.size(), 240);
  expect_eq(c.count(), 3);

  expect_eq(c.change_size("key4", 300), false);
  expect_eq(c.size(), 240);
  expect_eq(c.count(), 3);

  expect_eq(c.touch("key4", 300), false);
  expect_eq(c.size(), 240);
  expect_eq(c.count(), 3);

  expect(c.touch("key1", 100));
  expect_eq(c.size(), 260);
  expect_eq(c.count(), 3);

  LRUMap<string, string> d;
  expect_eq(d.size(), 0);
  expect_eq(d.count(), 0);

  d.swap(c);
  expect_eq(c.size(), 0);
  expect_eq(c.count(), 0);
  expect_eq(d.size(), 260);
  expect_eq(d.count(), 3);

  auto evicted = d.evict_object();
  expect_eq(evicted.key, "key2");
  expect_eq(evicted.value, "value2");
  expect_eq(evicted.size, 80);
  expect_eq(d.size(), 180);
  expect_eq(d.count(), 2);

  evicted = d.evict_object();
  expect_eq(evicted.key, "key3");
  expect_eq(evicted.value, "value3");
  expect_eq(evicted.size, 80);
  expect_eq(d.size(), 100);
  expect_eq(d.count(), 1);

  evicted = d.evict_object();
  expect_eq(evicted.key, "key1");
  expect_eq(evicted.value, "value1");
  expect_eq(evicted.size, 100);
  expect_eq(d.size(), 0);
  expect_eq(d.count(), 0);

  printf("LRUMapTest: all tests passed\n");

  return 0;
}
