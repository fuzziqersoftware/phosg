#pragma once

#include <stdint.h>

#include <unordered_map>


template <typename K>
class LRUSet {
protected:
  struct Item {
    Item* prev;
    Item* next;
    const K* key;
    size_t size;

    Item(size_t size);
  };

  Item* head;
  Item* tail;
  std::unordered_map<K, Item> items;
  size_t total_size;

  void unlink_item(Item* i);
  void link_item(Item* i);

public:
  LRUSet();
  virtual ~LRUSet();

  bool insert(const K& k, size_t size);
  bool emplace(K&& k, size_t size);

  bool erase(const K& k);

  bool change_size(const K& k, size_t new_size);
  bool touch(const K& k, ssize_t new_size = -1);

  size_t size() const;
  size_t count() const;

  std::pair<K, size_t> evict_object();

  void print() const;
};

#include "LRUSet-inl.hh"
