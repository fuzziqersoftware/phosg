#pragma once

#include <inttypes.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

template <typename CoordType, typename ValueType>
KDTree<CoordType, ValueType>::KDTree()
    : root(nullptr),
      node_count(0) {}

template <typename CoordType, typename ValueType>
KDTree<CoordType, ValueType>::~KDTree() {
  std::deque<Node*> to_delete;
  to_delete.emplace_back(this->root);
  while (!to_delete.empty()) {
    Node* n = to_delete.front();
    to_delete.pop_front();

    if (n->before) {
      to_delete.emplace_back(n->before);
    }
    if (n->after_or_equal) {
      to_delete.emplace_back(n->after_or_equal);
    }
    delete n;
  }
}

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Iterator
KDTree<CoordType, ValueType>::insert(const CoordType& pt, const ValueType& v) {
  Node* n = new Node(pt, v);
  this->link_node(n);
  return Iterator(n);
}

template <typename CoordType, typename ValueType>
template <typename... Args>
typename KDTree<CoordType, ValueType>::Iterator
KDTree<CoordType, ValueType>::emplace(const CoordType& pt, Args&&... args) {
  Node* n = new Node(pt, std::forward(args)...);
  this->link_node(n);
  return Iterator(n);
}

template <typename CoordType, typename ValueType>
void KDTree<CoordType, ValueType>::link_node(Node* new_node) {
  if (this->root == nullptr) {
    this->root = new_node;
    this->node_count++;
    return;
  }

  Node* n = this->root;
  for (;;) {
    if (new_node->pt.at(n->dim) < n->pt.at(n->dim)) {
      if (n->before == nullptr) {
        new_node->dim = (n->dim + 1) % CoordType::dimensions();
        new_node->parent = n;
        n->before = new_node;
        this->node_count++;
        return;
      } else {
        n = n->before;
      }
    } else {
      if (n->after_or_equal == nullptr) {
        new_node->dim = (n->dim + 1) % CoordType::dimensions();
        new_node->parent = n;
        n->after_or_equal = new_node;
        this->node_count++;
        return;
      } else {
        n = n->after_or_equal;
      }
    }
  }
}

template <typename CoordType, typename ValueType>
bool KDTree<CoordType, ValueType>::erase(
    const CoordType& pt, const ValueType& v) {
  if (this->root == nullptr) {
    return false;
  }

  for (Node* n = this->root; n != nullptr;) {
    if ((n->pt == pt) && (n->value == v)) {
      this->delete_node(n);
      return true;
    }
    n = (pt.at(n->dim) < n->pt.at(n->dim)) ? n->before : n->after_or_equal;
  }
  return false;
}

template <typename CoordType, typename ValueType>
void KDTree<CoordType, ValueType>::erase_advance(Iterator& it) {
  // delete_node doesn't actually delete the node unless it was a leaf node. if
  // it wasn't a leaf, its value got replaced with a node we haven't seen yet,
  // so just leave the queue alone. if it was a leaf, then it got deleted, so
  // remove the pointer from the queue and update current
  Node* to_delete = it.pending.front();
  bool deleted = this->delete_node(to_delete);
  if (deleted) {
    it.pending.pop_front();
    if (!it.pending.empty()) {
      Node* n = it.pending.front();
      it.current = std::make_pair(n->pt, n->value);
    }
  } else {
    Node* n = it.pending.front();
    it.current = std::make_pair(n->pt, n->value);
  }
}

template <typename CoordType, typename ValueType>
const ValueType& KDTree<CoordType, ValueType>::at(
    const CoordType& pt) const {
  for (Node* n = this->root; n;) {
    if (n->pt == pt) {
      return n->value;
    }
    n = (pt.at(n->dim) < n->pt.at(n->dim)) ? n->before : n->after_or_equal;
  }
  throw std::out_of_range("no such item");
}

template <typename CoordType, typename ValueType>
bool KDTree<CoordType, ValueType>::exists(const CoordType& pt) const {
  try {
    this->at(pt);
    return true;
  } catch (const std::out_of_range&) {
    return false;
  }
}

template <typename CoordType, typename ValueType>
typename std::vector<std::pair<CoordType, ValueType>>
KDTree<CoordType, ValueType>::within(
    const CoordType& low, const CoordType& high) const {
  if (this->root == nullptr) {
    throw std::out_of_range("no such item");
  }

  std::deque<Node*> level_nodes;
  level_nodes.emplace_back(this->root);
  std::vector<std::pair<CoordType, ValueType>> ret;
  while (!level_nodes.empty()) {
    Node* n = level_nodes.front();
    level_nodes.pop_front();

    // if this node is within the range, return it
    {
      size_t dim;
      for (dim = 0; dim < CoordType::dimensions(); dim++) {
        if ((n->pt.at(dim) < low.at(dim)) ||
            (n->pt.at(dim) >= high.at(dim))) {
          break;
        }
      }
      if (dim == CoordType::dimensions()) {
        ret.emplace_back(std::make_pair(n->pt, n->value));
      }
    }

    // if the range falls entirely on one side of the node, move down to that
    // node. otherwise, move down to both nodes
    bool low_less = (low.at(n->dim) < n->pt.at(n->dim));
    bool high_greater = (high.at(n->dim) >= n->pt.at(n->dim));
    if (low_less && n->before) {
      level_nodes.emplace_back(n->before);
    }
    if (high_greater && n->after_or_equal) {
      level_nodes.emplace_back(n->after_or_equal);
    }
  }
  return ret;
}

// TODO: somehow deduplicate this code with within()
template <typename CoordType, typename ValueType>
bool KDTree<CoordType, ValueType>::exists(const CoordType& low,
    const CoordType& high) const {
  if (this->root == nullptr) {
    return false;
  }

  std::deque<Node*> level_nodes;
  level_nodes.emplace_back(this->root);
  while (!level_nodes.empty()) {
    Node* n = level_nodes.front();
    level_nodes.pop_front();

    // if this node is within the range, return it
    {
      size_t dim;
      for (dim = 0; dim < CoordType::dimensions(); dim++) {
        if ((n->pt.at(dim) < low.at(dim)) ||
            (n->pt.at(dim) >= high.at(dim))) {
          break;
        }
      }
      if (dim == CoordType::dimensions()) {
        return true;
      }
    }

    // if the range falls entirely on one side of the node, move down to that
    // node. otherwise, move down to both nodes
    bool low_less = (low.at(n->dim) < n->pt.at(n->dim));
    bool high_greater = (high.at(n->dim) >= n->pt.at(n->dim));
    if (low_less && n->before) {
      level_nodes.emplace_back(n->before);
    }
    if (high_greater && n->after_or_equal) {
      level_nodes.emplace_back(n->after_or_equal);
    }
  }
  return false;
}

template <typename CoordType, typename ValueType>
size_t KDTree<CoordType, ValueType>::size() const {
  return this->node_count;
}

template <typename CoordType, typename ValueType>
size_t KDTree<CoordType, ValueType>::depth() const {
  return this->depth_recursive(this->root, 0);
}

template <typename CoordType, typename ValueType>
size_t KDTree<CoordType, ValueType>::depth_recursive(Node* n,
    size_t depth) {
  if (n == nullptr) {
    return depth;
  }
  size_t before_depth = KDTree::depth_recursive(n->before, depth + 1);
  size_t after_or_equal_depth = KDTree::depth_recursive(n->after_or_equal, depth + 1);
  return (before_depth > after_or_equal_depth) ? before_depth : after_or_equal_depth;
}

template <typename CoordType, typename ValueType>
KDTree<CoordType, ValueType>::Node::Node(
    Node* parent, const CoordType& pt, size_t dim, const ValueType& v)
    : pt(pt),
      dim(dim),
      before(nullptr),
      after_or_equal(nullptr),
      parent(parent),
      value(v) {}

template <typename CoordType, typename ValueType>
template <typename... Args>
KDTree<CoordType, ValueType>::Node::Node(const CoordType& pt, Args&&... args)
    : pt(pt),
      dim(0),
      before(nullptr),
      after_or_equal(nullptr),
      parent(nullptr),
      value(std::forward<Args>(args)...) {}

template <typename CoordType, typename ValueType>
size_t KDTree<CoordType, ValueType>::count_subtree(const Node* n) {
  // TODO: make this not recursive
  if (!n) {
    return 0;
  }
  return 1 + KDTree::count_subtree(n->before) + KDTree::count_subtree(n->after_or_equal);
}

template <typename CoordType, typename ValueType>
void KDTree<CoordType, ValueType>::collect_into(Node* n,
    std::vector<Node*>& ret) {
  if (!n) {
    return;
  }

  std::deque<Node*> level_nodes;
  level_nodes.emplace_back(n);

  while (!level_nodes.empty()) {
    Node* n = level_nodes.front();
    level_nodes.pop_front();

    ret.emplace_back(n);
    if (n->before) {
      level_nodes.emplace_back(n->before);
    }
    if (n->after_or_equal) {
      level_nodes.emplace_back(n->after_or_equal);
    }
  }
}

template <typename CoordType, typename ValueType>
bool KDTree<CoordType, ValueType>::delete_node(Node* n) {
  // replace the node with an appropriate node from its subtree, repeating until
  // the node is a leaf node
  bool was_leaf_node = true;
  while (n->before || n->after_or_equal) {
    was_leaf_node = false;
    Node* target;
    if (n->before) {
      target = KDTree::find_subtree_min_max(n->before, n->dim, true);
    } else if (n->after_or_equal) {
      target = KDTree::find_subtree_min_max(n->after_or_equal, n->dim, false);
    } else {
      throw std::logic_error("node is a leaf but still claims to be movable");
    }
    n->pt = target->pt;
    n->value = std::move(target->value);
    n = target;
  }

  // unlink the node from its parent
  if (n->parent == nullptr) {
    this->root = nullptr;
  } else {
    if (n->parent->before == n) {
      n->parent->before = nullptr;
    }
    if (n->parent->after_or_equal == n) {
      n->parent->after_or_equal = nullptr;
    }
  }

  // delete the (now-leaf) node
  this->node_count--;
  delete n;

  return was_leaf_node;
}

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Node*
KDTree<CoordType, ValueType>::find_subtree_min_max(Node* n,
    size_t target_dim, bool find_max) {

  Node* ret = n;

  std::deque<Node*> pending;
  pending.emplace_back(n);
  while (!pending.empty()) {
    n = pending.front();
    pending.pop_front();

    // update the min/max if this point is more extreme
    if (find_max) {
      if (n->pt.at(target_dim) > ret->pt.at(target_dim)) {
        ret = n;
      }
    } else {
      if (n->pt.at(target_dim) < ret->pt.at(target_dim)) {
        ret = n;
      }
    }

    // add new points to pending if needed. if this point splits the space along
    // the dimension we care about, then we only need to look at one side;
    // otherwise we have to look at both
    if (((n->dim != target_dim) || !find_max) && n->before) {
      pending.emplace_back(n->before);
    }
    if (((n->dim != target_dim) || find_max) && n->after_or_equal) {
      pending.emplace_back(n->after_or_equal);
    }
  }

  return ret;
}

/* this implementation doesn't work; keeping it for reference just in case I
 * feel like fixing it. the problem is that used nodes need to be eliminated
 * from all nodes_by_dim vectors, not only the current one. maybe could fix this
 * by marking nodes somehow (maybe using the dim field) but this would be hacky

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Node*
KDTree<CoordType, ValueType>::delete_node(Node* n) {

  // collect the nodes we want to rebuild the subtree under
  std::vector<Node*> nodes_by_dim[CoordType::dimensions()];
  KDTree::collect_into(n->before, nodes_by_dim[0]);
  KDTree::collect_into(n->after_or_equal, nodes_by_dim[0]);
  for (size_t x = 1; x < CoordType::dimensions(); x++) {
    nodes_by_dim[x] = nodes_by_dim[0];
  }

  // we're done with the node; delete it
  size_t dim = n->dim;
  this->node_count--;
  delete n;

  // generate sorted lists for each dimension
  size_t min_by_dim[CoordType::dimensions()];
  size_t max_by_dim[CoordType::dimensions()];
  for (size_t x = 0; x < CoordType::dimensions(); x++) {
    std::sort(nodes_by_dim[x].begin(), nodes_by_dim[x].end(), [&](Node* a, Node* b) {
      return (a->pt.coords[x] < b->pt.coords[x]);
    });
    min_by_dim[x] = 0;
    max_by_dim[x] = nodes_by_dim[x].size();
  }

  // generate subtrees recursively
  return KDTree::generate_balanced_subtree(nodes_by_dim, min_by_dim,
      max_by_dim, dim);
}

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Node*
KDTree<CoordType, ValueType>::generate_balanced_subtree(
    const std::vector<Node*>* nodes_by_dim, size_t* min_by_dim,
    size_t* max_by_dim, size_t dim) {
  size_t& min = min_by_dim[dim];
  size_t& max = max_by_dim[dim];

  if (min >= max) {
    return nullptr;
  }
  size_t mid = min + (max - min) / 2;
  Node* new_root = nodes_by_dim[dim][mid];

  size_t next_dim = (dim + 1) % CoordType::dimensions();
  size_t orig_min = min;
  min = mid + 1; // min is a reference, so this changes min_x or min_y
  new_root->after_or_equal = generate_balanced_subtree(nodes_by_dim, min_by_dim,
      max_by_dim, next_dim);
  if (new_root->after_or_equal) {
    new_root->after_or_equal->parent = new_root;
  }
  min = orig_min;
  max = mid;
  new_root->before = generate_balanced_subtree(nodes_by_dim, min_by_dim,
      max_by_dim, next_dim);
  if (new_root->before) {
    new_root->before->parent = new_root;
  }
  new_root->dim = dim;

  return new_root;
} */

template <typename CoordType, typename ValueType>
KDTree<CoordType, ValueType>::Iterator::Iterator(Node* n) {
  if (n) {
    this->pending.emplace_back(n);
    this->current = std::make_pair(n->pt, n->value);
  }
}

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Iterator&
KDTree<CoordType, ValueType>::Iterator::operator++() {
  const Node* n = this->pending.front();
  this->pending.pop_front();

  if (n->before) {
    this->pending.emplace_back(n->before);
  }
  if (n->after_or_equal) {
    this->pending.emplace_back(n->after_or_equal);
  }

  if (!this->pending.empty()) {
    n = this->pending.front();
    this->current = std::make_pair(n->pt, n->value);
  }

  return *this;
}

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Iterator
KDTree<CoordType, ValueType>::Iterator::operator++(int) {
  Iterator ret = *this;
  this->operator++();
  return ret;
}

template <typename CoordType, typename ValueType>
bool KDTree<CoordType, ValueType>::Iterator::operator==(
    const Iterator& other) const {
  bool this_empty = this->pending.empty();
  bool other_empty = other.pending.empty();
  if (this_empty && other_empty) {
    return true;
  }
  if (this_empty != other_empty) {
    return false;
  }
  return this->pending.front() == other.pending.front();
}

template <typename CoordType, typename ValueType>
bool KDTree<CoordType, ValueType>::Iterator::operator!=(
    const Iterator& other) const {
  return !this->operator==(other);
}

template <typename CoordType, typename ValueType>
const typename std::pair<CoordType, ValueType>&
KDTree<CoordType, ValueType>::Iterator::operator*() const {
  return this->current;
}

template <typename CoordType, typename ValueType>
const typename std::pair<CoordType, ValueType>*
KDTree<CoordType, ValueType>::Iterator::operator->() const {
  return &this->current;
}

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Iterator
KDTree<CoordType, ValueType>::begin() const {
  return Iterator(this->root);
}

template <typename CoordType, typename ValueType>
typename KDTree<CoordType, ValueType>::Iterator
KDTree<CoordType, ValueType>::end() const {
  return Iterator(nullptr);
}
