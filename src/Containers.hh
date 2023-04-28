#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <typename V>
std::vector<V> set_to_vec(const std::set<V>& s) {
  std::vector<V> ret;
  ret.reserve(s.size());
  for (const auto& x : s) {
    ret.emplace_back(x);
  }
  return ret;
}

template <typename V>
std::vector<V> set_to_vec(const std::unordered_set<V>& s) {
  std::vector<V> ret;
  ret.reserve(s.size());
  for (const auto& x : s) {
    ret.emplace_back(x);
  }
  return ret;
}

template <typename V>
std::set<V> vec_to_set(const std::vector<V>& s) {
  std::set<V> ret;
  for (const auto& x : s) {
    ret.emplace_back(x);
  }
  return ret;
}

template <typename V>
std::unordered_set<V> vec_to_uset(const std::vector<V>& s) {
  std::unordered_set<V> ret;
  for (const auto& x : s) {
    ret.emplace_back(x);
  }
  return ret;
}

template <typename K, typename V>
std::vector<K> map_keys_to_vec(const std::map<K, V>& s) {
  std::vector<K> ret;
  ret.reserve(s.size());
  for (const auto& it : s) {
    ret.emplace_back(it.first);
  }
  return ret;
}

template <typename K, typename V>
std::vector<K> map_keys_to_vec(const std::unordered_map<K, V>& s) {
  std::vector<K> ret;
  ret.reserve(s.size());
  for (const auto& it : s) {
    ret.emplace_back(it.first);
  }
  return ret;
}

template <typename K, typename V>
std::vector<K> map_values_to_vec(const std::map<K, V>& s) {
  std::vector<K> ret;
  ret.reserve(s.size());
  for (const auto& it : s) {
    ret.emplace_back(it.second);
  }
  return ret;
}

template <typename K, typename V>
std::vector<K> map_values_to_vec(const std::unordered_map<K, V>& s) {
  std::vector<K> ret;
  ret.reserve(s.size());
  for (const auto& it : s) {
    ret.emplace_back(it.second);
  }
  return ret;
}
