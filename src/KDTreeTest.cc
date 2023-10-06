#include "KDTree.hh"

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include <set>
#include <string>

#include "UnitTest.hh"
#include "Vector.hh"

using namespace std;

void run_randomized_test() {
  printf("-- randomized\n");

  printf("--   construction/insert\n");
  srand(time(nullptr));
  KDTree<Vector2<int64_t>, int64_t> t;
  set<pair<int64_t, int64_t>> points;
  while (points.size() < 1000) {
    int64_t x = rand() % 1000;
    int64_t y = rand() % 1000;
    if (points.emplace(make_pair(x, y)).second) {
      t.insert({x, y}, x * y);
    }
  }
  fprintf(stderr, "--     tree: size=%zu, depth=%zu\n", t.size(), t.depth());

  printf("--   within\n");
  {
    set<pair<int64_t, int64_t>> remaining_points = points;
    for (const auto& it : t.within({250, 250}, {750, 750})) {
      expect_ge(it.first.x, 250);
      expect_lt(it.first.x, 750);
      expect_ge(it.first.y, 250);
      expect_lt(it.first.y, 750);
      expect_eq(1, remaining_points.erase(make_pair(it.first.x, it.first.y)));
    }
    for (const auto& it : remaining_points) {
      expect((it.first < 250) || (it.first >= 750) || (it.second < 250) || (it.second >= 750));
    }
  }

  printf("--   erase 1\n");
  {
    // delete all points where x + y is an odd number
    for (auto it = t.begin(); it != t.end();) {
      expect_eq(1, points.count(make_pair(it->first.x, it->first.y)));
      if ((it->first.x + it->first.y) % 2) {
        t.erase_advance(it);
      } else {
        ++it;
      }
    }
    fprintf(stderr, "--     tree: size=%zu, depth=%zu\n", t.size(), t.depth());
    for (auto it : t) {
      expect_eq(1, points.count(make_pair(it.first.x, it.first.y)));
      expect_eq(0, (it.first.x + it.first.y) % 2);
    }
  }

  printf("--   erase 2\n");
  {
    // delete all points in (250, 250) -> (750, 750)
    for (auto it = t.begin(); it != t.end();) {
      expect_eq(1, points.count(make_pair(it->first.x, it->first.y)));
      if ((it->first.x >= 250) && (it->first.y >= 250) &&
          (it->first.x < 750) && (it->first.y < 750)) {
        t.erase_advance(it);
      } else {
        ++it;
      }
    }
    fprintf(stderr, "--     tree: size=%zu, depth=%zu\n", t.size(), t.depth());
    for (auto it : t) {
      expect_eq(1, points.count(make_pair(it.first.x, it.first.y)));
      expect_eq(0, (it.first.x + it.first.y) % 2);
    }
  }

  expect_eq(false, t.exists({250, 250}, {750, 750}));
  expect_eq(true, t.exists({0, 0}, {1000, 1000}));
}

void run_basic_test() {
  printf("-- basic\n");

  printf("--   construction\n");

  KDTree<Vector2<int64_t>, int64_t> t;
  expect_eq(t.size(), 0);

  struct Entry {
    int64_t x;
    int64_t y;
    int64_t v;

    bool operator<(const Entry& other) const {
      if (this->x < other.x) {
        return true;
      }
      if (this->x > other.x) {
        return false;
      }
      if (this->y < other.y) {
        return true;
      }
      if (this->y > other.y) {
        return false;
      }
      return (this->v < other.v);
    }
  };
  const vector<Entry> entries({
      {2, 3, 0},
      {5, 4, 1},
      {9, 6, 2},
      {4, 7, 3},
      {8, 1, 4},
      {7, 2, 5},
  });

  printf("--   insert\n");
  t.insert({2, 3}, 0);
  t.insert({5, 4}, 1);
  t.insert({9, 6}, 2);
  t.insert({4, 7}, 3);
  t.insert({8, 1}, 4);
  t.insert({7, 2}, 5);

  printf("--   at/exists\n");
  expect_eq(6, t.size());
  expect_eq(4, t.at({8, 1}));
  expect_raises<out_of_range>([&]() {
    t.at({8, 2});
  });
  expect_eq(true, t.exists({5, 4}));
  expect_eq(false, t.exists({5, 3}));

  printf("--   within\n");
  {
    set<Entry> remaining_entries;
    remaining_entries.insert({2, 3, 0});
    remaining_entries.insert({4, 7, 3});
    for (const auto& it : t.within({0, 0}, {5, 10})) {
      expect_eq(1, remaining_entries.erase({it.first.x, it.first.y, it.second}));
    }
    expect_eq(0, remaining_entries.size());
  }

  printf("--   erase\n");
  expect_eq(false, t.erase({8, 6}, 2));
  expect_eq(6, t.size());
  expect_eq(false, t.erase({9, 6}, 1));
  expect_eq(6, t.size());
  expect_eq(true, t.erase({9, 6}, 2));
  expect_eq(5, t.size());

  printf("--   iterate\n");
  {
    set<Entry> remaining_entries(entries.begin(), entries.end());
    expect_eq(1, remaining_entries.erase({9, 6, 2}));
    for (const auto& it : t) {
      expect_eq(1, remaining_entries.erase({it.first.x, it.first.y, it.second}));
    }
    expect_eq(0, remaining_entries.size());
  }

  printf("--   iterate+erase\n");
  {
    set<Entry> remaining_entries(entries.begin(), entries.end());
    expect_eq(1, remaining_entries.erase({9, 6, 2}));
    for (auto it = t.begin(); it != t.end();) {
      expect_eq(1, remaining_entries.erase({it->first.x, it->first.y, it->second}));
      if (it->first.x == 4) {
        t.erase_advance(it);
      } else {
        ++it;
      }
    }
    expect_eq(0, remaining_entries.size());
  }

  printf("--   ensure committed (iterate)\n");
  {
    set<Entry> remaining_entries(entries.begin(), entries.end());
    expect_eq(1, remaining_entries.erase({9, 6, 2}));
    expect_eq(1, remaining_entries.erase({4, 7, 3}));
    for (const auto& it : t) {
      expect_eq(1, remaining_entries.erase({it.first.x, it.first.y, it.second}));
    }
    expect_eq(0, remaining_entries.size());
  }

  printf("--   ensure committed (within)\n");
  {
    set<Entry> remaining_entries;
    remaining_entries.insert({2, 3, 0});
    for (const auto& it : t.within({0, 0}, {5, 10})) {
      expect_eq(1, remaining_entries.erase({it.first.x, it.first.y, it.second}));
    }
    expect_eq(0, remaining_entries.size());
  }
}

int main(int, char**) {
  run_basic_test();
  run_randomized_test();
  printf("KDTreeTest: all tests passed\n");
  return 0;
}
