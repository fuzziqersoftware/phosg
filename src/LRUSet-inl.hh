#include <stdint.h>

#include <stdexcept>
#include <unordered_map>
#include <utility>

template <typename K>
LRUSet<K>::Item::Item(size_t size)
    : prev(nullptr),
      next(nullptr),
      key(nullptr),
      size(size) {}

template <typename K>
LRUSet<K>::LRUSet()
    : head(nullptr),
      tail(nullptr),
      items(),
      total_size(0) {}

template <typename K>
LRUSet<K>::~LRUSet() {
  // don't need to do anything - the items and keys will be destroyed by the
  // unordered_map destructor
}

template <typename K>
bool LRUSet<K>::after_emplace(
    const std::pair<typename std::unordered_map<K, Item>::iterator, bool>& emplace_ret, size_t size) {
  auto& k = emplace_ret.first->first;
  auto& i = emplace_ret.first->second;

  if (emplace_ret.second) {
    // item was inserted
    i.key = &k;
    i.size = size;
    this->total_size += size;
    this->link_item(&i);
    return true;

  } else {
    // item already existed. the key must match already; just update the size
    // and move the item to the front of the lru
    ssize_t size_delta = static_cast<ssize_t>(size) - static_cast<ssize_t>(i.size);
    i.size = size;
    this->total_size += size_delta;
    this->unlink_item(&i);
    this->link_item(&i);
    return false;
  }
}

template <typename K>
bool LRUSet<K>::insert(const K& key, size_t size) {
  auto emplace_ret = this->items.emplace(std::piecewise_construct,
      std::make_tuple(key), std::make_tuple(size));
  return this->after_emplace(emplace_ret, size);
}

template <typename K>
bool LRUSet<K>::emplace(K&& key, size_t size) {
  auto emplace_ret = this->items.emplace(std::piecewise_construct,
      std::forward_as_tuple(std::move(key)), std::forward_as_tuple(size));
  return this->after_emplace(emplace_ret, size);
}

template <typename K>
bool LRUSet<K>::erase(const K& k) {
  auto item_it = this->items.find(k);
  if (item_it == this->items.end()) {
    return false;
  }

  Item& item = item_it->second;
  this->unlink_item(&item);
  this->total_size -= item.size;
  this->items.erase(item_it);
  return true;
}

template <typename K>
void LRUSet<K>::clear() {
  this->head = nullptr;
  this->tail = nullptr;
  this->items.clear();
  this->total_size = 0;
}

template <typename K>
bool LRUSet<K>::change_size(const K& k, size_t new_size) {
  try {
    Item& i = this->items.at(k);
    this->total_size += new_size - i.size;
    i.size = new_size;
    return true;
  } catch (const std::out_of_range& e) {
    return false;
  }
}

template <typename K>
bool LRUSet<K>::touch(const K& k, ssize_t new_size) {
  try {
    Item& i = this->items.at(k);
    if (this->head != &i) {
      this->unlink_item(&i);
      this->link_item(&i);
    }
    if (new_size >= 0) {
      this->total_size += new_size - i.size;
      i.size = new_size;
    }
    return true;
  } catch (const std::out_of_range& e) {
    return false;
  }
}

template <typename K>
void LRUSet<K>::unlink_item(Item* i) {
  if (this->head == i) {
    this->head = i->next;
  }
  if (this->tail == i) {
    this->tail = i->prev;
  }
  if (i->prev) {
    i->prev->next = i->next;
  }
  if (i->next) {
    i->next->prev = i->prev;
  }
  i->prev = nullptr;
  i->next = nullptr;
}

template <typename K>
void LRUSet<K>::link_item(Item* i) {
  i->next = this->head;
  if (this->head) {
    this->head->prev = i;
  }
  this->head = i;
  if (!this->tail) {
    this->tail = i;
  }
}

template <typename K>
size_t LRUSet<K>::size() const {
  return total_size;
}

template <typename K>
size_t LRUSet<K>::count() const {
  return this->items.size();
}

template <typename K>
std::pair<K, size_t> LRUSet<K>::evict_object() {
  Item* i = this->tail;
  if (!i) {
    throw std::out_of_range("nothing to evict");
  }

  this->unlink_item(i);
  std::pair<K, size_t> ret = std::make_pair(*i->key, i->size);

  this->items.erase(ret.first);
  this->total_size -= ret.second;
  return ret;
}

template <typename K>
std::pair<K, size_t> LRUSet<K>::peek() {
  Item* i = this->tail;
  if (!i) {
    throw std::out_of_range("set is empty");
  }
  return std::make_pair(*i->key, i->size);
}

template <typename K>
void LRUSet<K>::swap(LRUSet<K>& other) {
  Item* this_head = this->head;
  Item* this_tail = this->tail;
  size_t this_total_size = this->total_size;

  this->items.swap(other.items);
  this->head = other.head;
  this->tail = other.tail;
  this->total_size = other.total_size;

  other.head = this_head;
  other.tail = this_tail;
  other.total_size = this_total_size;
}
