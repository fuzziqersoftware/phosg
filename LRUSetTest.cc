#include <assert.h>
#include <stdio.h>

#include <string>

#include "LRUSet.hh"

using namespace std;


int main(int argc, char** argv) {
  LRUSet<string> c;

  assert(c.size() == 0);
  assert(c.count() == 0);

  assert(c.insert("key1", 40));
  assert(c.size() == 40);
  assert(c.count() == 1);

  assert(c.emplace("key2", 80));
  assert(c.size() == 120);
  assert(c.count() == 2);

  assert(c.emplace("key3", 80));
  assert(c.size() == 200);
  assert(c.count() == 3);

  assert(c.change_size("key1", 80));
  assert(c.size() == 240);
  assert(c.count() == 3);

  assert(c.emplace("key2", 100) == false);
  assert(c.size() == 260);
  assert(c.count() == 3);

  assert(c.change_size("key4", 300) == false);
  assert(c.size() == 260);
  assert(c.count() == 3);

  assert(c.touch("key4", 300) == false);
  assert(c.size() == 260);
  assert(c.count() == 3);

  assert(c.touch("key1", 100));
  assert(c.size() == 280);
  assert(c.count() == 3);

  auto evicted = c.evict_object();
  assert(evicted.first == "key3");
  assert(evicted.second == 80);
  assert(c.size() == 200);
  assert(c.count() == 2);

  evicted = c.evict_object();
  assert(evicted.first == "key2");
  assert(evicted.second == 100);
  assert(c.size() == 100);
  assert(c.count() == 1);

  evicted = c.evict_object();
  assert(evicted.first == "key1");
  assert(evicted.second == 100);
  assert(c.size() == 0);
  assert(c.count() == 0);

  printf("%s: all tests passed\n", argv[0]);

  return 0;
}
