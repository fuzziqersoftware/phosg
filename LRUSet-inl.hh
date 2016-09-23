#include <stdint.h>

#include <stdexcept>
#include <unordered_map>


template <typename K>
LRUSet<K>::Item::Item(size_t size) :
    prev(NULL), next(NULL), key(NULL), size(size) { }

template <typename K>
LRUSet<K>::LRUSet() :
    head(NULL), tail(NULL), items(), total_size(0) { }

template <typename K>
LRUSet<K>::~LRUSet() {
  // don't need to do anything - the items and keys will be destroyed by the
  // unordered_map destructor
}

template <typename K>
bool LRUSet<K>::insert(const K& key, size_t size) {
  bool ret = this->erase(key);

  auto emplace_ret = this->items.emplace(std::piecewise_construct,
      std::make_tuple(key), std::make_tuple(size));
  auto& k = emplace_ret.first->first;
  auto& i = emplace_ret.first->second;

  i.key = &k;
  i.size = size;
  this->link_item(&i);

  this->total_size += size;

  return !ret;
}

template <typename K>
bool LRUSet<K>::emplace(K&& key, size_t size) {
  bool ret = this->erase(key);

  auto emplace_ret = this->items.emplace(std::piecewise_construct,
      std::make_tuple(move(key)), std::make_tuple(size));
  auto& k = emplace_ret.first->first;
  auto& i = emplace_ret.first->second;

  i.key = &k;
  i.size = size;
  this->link_item(&i);

  this->total_size += size;

  return !ret;
}

template <typename K>
bool LRUSet<K>::erase(const K& k) {
  try {
    Item& i = this->items.at(k);
    this->unlink_item(&i);
    this->total_size -= i.size;
    this->items.erase(k);
    return true;
  } catch (const std::out_of_range& e) {
    return false;
  }
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
    this->unlink_item(&i);
    this->link_item(&i);
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
  if (i->prev) {
    i->prev->next = i->next;
  }
  if (i->next) {
    i->next->prev = i->prev;
  }
  if (this->head == i) {
    this->head = i->next;
  }
  if (this->tail == i) {
    this->tail = i->prev;
  }
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
    throw std::out_of_range("");
  }

  this->unlink_item(i);
  std::pair<K, size_t> ret = std::make_pair(*i->key, i->size);

  this->items.erase(ret.first);
  this->total_size -= ret.second;
  return ret;
}
