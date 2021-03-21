#pragma once

#include <inttypes.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>



template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::Point::operator==(const Point& other) {
  for (size_t x = 0; x < dimensions; x++) {
    if (this->coords[x] != other.coords[x]) {
      return false;
    }
  }
  return true;
}

template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::Point::operator!=(const Point& other) {
  return !this->operator==(other);
}



template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Point
KDTree<CoordType, dimensions, ValueType>::make_point(CoordType x) {
  if (dimensions != 1) {
    throw std::logic_error("inorrect dimensional point constructor called");
  }
  Point pt;
  pt.coords[0] = x;
  return pt;
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Point
KDTree<CoordType, dimensions, ValueType>::make_point(CoordType x, CoordType y) {
  if (dimensions != 2) {
    throw std::logic_error("inorrect dimensional point constructor called");
  }
  Point pt;
  pt.coords[0] = x;
  pt.coords[1] = y;
  return pt;
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Point
KDTree<CoordType, dimensions, ValueType>::make_point(CoordType x, CoordType y,
    CoordType z) {
  if (dimensions != 3) {
    throw std::logic_error("inorrect dimensional point constructor called");
  }
  Point pt;
  pt.coords[0] = x;
  pt.coords[1] = y;
  pt.coords[2] = z;
  return pt;
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Point
KDTree<CoordType, dimensions, ValueType>::make_point(CoordType x, CoordType y,
    CoordType z, CoordType w) {
  if (dimensions != 4) {
    throw std::logic_error("inorrect dimensional point constructor called");
  }
  Point pt;
  pt.coords[0] = x;
  pt.coords[1] = y;
  pt.coords[2] = z;
  pt.coords[3] = w;
  return pt;
}



template <typename CoordType, size_t dimensions, typename ValueType>
KDTree<CoordType, dimensions, ValueType>::KDTree()
  : root(NULL), node_count(0) { }

template <typename CoordType, size_t dimensions, typename ValueType>
KDTree<CoordType, dimensions, ValueType>::~KDTree() {
  std::deque<Node*> to_delete;
  to_delete.emplace_back(this->root);
  while (!to_delete.empty()) {
    Node* n = to_delete.front();
    to_delete.pop_front();

    if (n->before) {
      to_delete.emplace_back(n->before);
    }
    if (n->after) {
      to_delete.emplace_back(n->after);
    }
    delete n;
  }
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Iterator
KDTree<CoordType, dimensions, ValueType>::insert(const Point& pt,
    const ValueType& v) {
  Node* n = new Node(pt, v);
  this->link_node(n);
  return Iterator(n);
}

template <typename CoordType, size_t dimensions, typename ValueType>
template <typename... Args>
typename KDTree<CoordType, dimensions, ValueType>::Iterator
KDTree<CoordType, dimensions, ValueType>::emplace(
    const Point& pt, Args&&... args) {
  Node* n = new Node(pt, std::forward(args)...);
  this->link_node(n);
  return Iterator(n);
}

template <typename CoordType, size_t dimensions, typename ValueType>
void KDTree<CoordType, dimensions, ValueType>::link_node(Node* new_node) {
  if (this->root == NULL) {
    this->root = new_node;
    this->node_count++;
    return;
  }

  Node* n = this->root;
  for (;;) {
    if (new_node->pt.coords[n->dim] < n->pt.coords[n->dim]) {
      if (n->before == NULL) {
        new_node->dim = (n->dim + 1) % dimensions;
        new_node->parent = n;
        n->before = new_node;
        this->node_count++;
        return;
      } else {
        n = n->before;
      }
    } else {
      if (n->after == NULL) {
        new_node->dim = (n->dim + 1) % dimensions;
        new_node->parent = n;
        n->after = new_node;
        this->node_count++;
        return;
      } else {
        n = n->after;
      }
    }
  }
}

template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::erase(
    const Point& pt, const ValueType& v) {
  if (this->root == NULL) {
    return false;
  }

  for (Node* n = this->root; n != NULL;) {
    if ((n->pt == pt) && (n->value == v)) {
      this->delete_node(n);
      return true;
    }
    n = (pt.coords[n->dim] < n->pt.coords[n->dim]) ? n->before : n->after;
  }
  return false;
}

template <typename CoordType, size_t dimensions, typename ValueType>
void KDTree<CoordType, dimensions, ValueType>::erase_advance(Iterator& it) {
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

template <typename CoordType, size_t dimensions, typename ValueType>
const ValueType& KDTree<CoordType, dimensions, ValueType>::at(
    const Point& pt) const {
  for (Node* n = this->root; n;) {
    if (n->pt == pt) {
      return n->value;
    }
    n = (pt.coords[n->dim] < n->pt.coords[n->dim]) ? n->before : n->after;
  }
  throw std::out_of_range("no such item");
}

template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::exists(const Point& pt) const {
  try {
    this->at(pt);
    return true;
  } catch (const std::out_of_range&) {
    return false;
  }
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename std::vector<std::pair<typename KDTree<CoordType, dimensions, ValueType>::Point, ValueType>>
KDTree<CoordType, dimensions, ValueType>::within(
    const Point& low, const Point& high) const {
  if (this->root == NULL) {
    throw std::out_of_range("no such item");
  }

  std::deque<Node*> level_nodes;
  level_nodes.emplace_back(this->root);
  std::vector<std::pair<Point, ValueType>> ret;
  while (!level_nodes.empty()) {
    Node* n = level_nodes.front();
    level_nodes.pop_front();

    // if this node is within the range, return it
    {
      size_t dim;
      for (dim = 0; dim < dimensions; dim++) {
        if ((n->pt.coords[dim] < low.coords[dim]) ||
            (n->pt.coords[dim] >= high.coords[dim])) {
          break;
        }
      }
      if (dim == dimensions) {
        ret.emplace_back(std::make_pair(n->pt, n->value));
      }
    }

    // if the range falls entirely on one side of the node, move down to that
    // node. otherwise, move down to both nodes
    bool low_less = (low.coords[n->dim] < n->pt.coords[n->dim]);
    bool high_greater = (high.coords[n->dim] >= n->pt.coords[n->dim]);
    if (low_less && n->before) {
      level_nodes.emplace_back(n->before);
    }
    if (high_greater && n->after) {
      level_nodes.emplace_back(n->after);
    }
  }
  return ret;
}

// TODO: somehow deduplicate this code with within()
template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::exists(const Point& low,
    const Point& high) const {
  if (this->root == NULL) {
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
      for (dim = 0; dim < dimensions; dim++) {
        if ((n->pt.coords[dim] < low.coords[dim]) ||
            (n->pt.coords[dim] >= high.coords[dim])) {
          break;
        }
      }
      if (dim == dimensions) {
        return true;
      }
    }

    // if the range falls entirely on one side of the node, move down to that
    // node. otherwise, move down to both nodes
    bool low_less = (low.coords[n->dim] < n->pt.coords[n->dim]);
    bool high_greater = (high.coords[n->dim] >= n->pt.coords[n->dim]);
    if (low_less && n->before) {
      level_nodes.emplace_back(n->before);
    }
    if (high_greater && n->after) {
      level_nodes.emplace_back(n->after);
    }
  }
  return false;
}

template <typename CoordType, size_t dimensions, typename ValueType>
size_t KDTree<CoordType, dimensions, ValueType>::size() const {
  return this->node_count;
}

template <typename CoordType, size_t dimensions, typename ValueType>
size_t KDTree<CoordType, dimensions, ValueType>::depth() const {
  return this->depth_recursive(this->root, 0);
}

template <typename CoordType, size_t dimensions, typename ValueType>
size_t KDTree<CoordType, dimensions, ValueType>::depth_recursive(Node* n,
    size_t depth) {
  if (n == NULL) {
    return depth;
  }
  size_t before_depth = KDTree::depth_recursive(n->before, depth + 1);
  size_t after_depth = KDTree::depth_recursive(n->after, depth + 1);
  return (before_depth > after_depth) ? before_depth : after_depth;
}



template <typename CoordType, size_t dimensions, typename ValueType>
KDTree<CoordType, dimensions, ValueType>::Node::Node(
    Node* parent, const Point& pt, size_t dim, const ValueType& v) : pt(pt),
      dim(dim), before(NULL), after(NULL), parent(parent), value(v) { }

template <typename CoordType, size_t dimensions, typename ValueType>
template <typename... Args>
KDTree<CoordType, dimensions, ValueType>::Node::Node(
    const Point& pt, Args&&... args) : pt(pt), dim(0), before(NULL),
    after(NULL), parent(NULL), value(std::forward<Args>(args)...) { }



template <typename CoordType, size_t dimensions, typename ValueType>
size_t KDTree<CoordType, dimensions, ValueType>::count_subtree(const Node* n) {
  // TODO: make this not recursive
  if (!n) {
    return 0;
  }
  return 1 + KDTree::count_subtree(n->before) + KDTree::count_subtree(n->after);
}

template <typename CoordType, size_t dimensions, typename ValueType>
void KDTree<CoordType, dimensions, ValueType>::collect_into(Node* n,
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
    if (n->after) {
      level_nodes.emplace_back(n->after);
    }
  }
}

template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::delete_node(Node* n) {
  // replace the node with an appropriate node from its subtree, repeating until
  // the node is a leaf node
  bool was_leaf_node = true;
  while (n->before || n->after) {
    was_leaf_node = false;
    Node* target;
    if (n->before) {
      target = KDTree::find_subtree_min_max(n->before, n->dim, true);
    } else if (n->after) {
      target = KDTree::find_subtree_min_max(n->after, n->dim, false);
    } else {
      throw std::logic_error("node is a leaf but still claims to be movable");
    }
    n->pt = target->pt;
    n->value = std::move(target->value);
    n = target;
  }

  // unlink the node from its parent
  if (n->parent == NULL) {
    this->root = NULL;
  } else {
    if (n->parent->before == n) {
      n->parent->before = NULL;
    }
    if (n->parent->after == n) {
      n->parent->after = NULL;
    }
  }

  // delete the (now-leaf) node
  this->node_count--;
  delete n;

  return was_leaf_node;
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Node*
KDTree<CoordType, dimensions, ValueType>::find_subtree_min_max(Node* n,
    size_t target_dim, bool find_max) {

  Node* ret = n;

  std::deque<Node*> pending;
  pending.emplace_back(n);
  while (!pending.empty()) {
    n = pending.front();
    pending.pop_front();

    // update the min/max if this point is more extreme
    if (find_max) {
      if (n->pt.coords[target_dim] > ret->pt.coords[target_dim]) {
        ret = n;
      }
    } else {
      if (n->pt.coords[target_dim] < ret->pt.coords[target_dim]) {
        ret = n;
      }
    }

    // add new points to pending if needed. if this point splits the space along
    // the dimension we care about, then we only need to look at one side;
    // otherwise we have to look at both
    if (((n->dim != target_dim) || !find_max) && n->before) {
      pending.emplace_back(n->before);
    }
    if (((n->dim != target_dim) || find_max) && n->after) {
      pending.emplace_back(n->after);
    }
  }

  return ret;
}



/* this implementation doesn't work; keeping it for reference just in case I
 * feel like fixing it. the problem is that used nodes need to be eliminated
 * from all nodes_by_dim vectors, not only the current one. maybe could fix this
 * by marking nodes somehow (maybe using the dim field) but this would be hacky

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Node*
KDTree<CoordType, dimensions, ValueType>::delete_node(Node* n) {

  // collect the nodes we want to rebuild the subtree under
  std::vector<Node*> nodes_by_dim[dimensions];
  KDTree::collect_into(n->before, nodes_by_dim[0]);
  KDTree::collect_into(n->after, nodes_by_dim[0]);
  for (size_t x = 1; x < dimensions; x++) {
    nodes_by_dim[x] = nodes_by_dim[0];
  }

  // we're done with the node; delete it
  size_t dim = n->dim;
  this->node_count--;
  delete n;

  // generate sorted lists for each dimension
  size_t min_by_dim[dimensions];
  size_t max_by_dim[dimensions];
  for (size_t x = 0; x < dimensions; x++) {
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

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Node*
KDTree<CoordType, dimensions, ValueType>::generate_balanced_subtree(
    const std::vector<Node*>* nodes_by_dim, size_t* min_by_dim,
    size_t* max_by_dim, size_t dim) {
  size_t& min = min_by_dim[dim];
  size_t& max = max_by_dim[dim];

  if (min >= max) {
    return NULL;
  }
  size_t mid = min + (max - min) / 2;
  Node* new_root = nodes_by_dim[dim][mid];

  size_t next_dim = (dim + 1) % dimensions;
  size_t orig_min = min;
  min = mid + 1; // min is a reference, so this changes min_x or min_y
  new_root->after = generate_balanced_subtree(nodes_by_dim, min_by_dim,
      max_by_dim, next_dim);
  if (new_root->after) {
    new_root->after->parent = new_root;
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



template <typename CoordType, size_t dimensions, typename ValueType>
KDTree<CoordType, dimensions, ValueType>::Iterator::Iterator(Node* n) {
  if (n) {
    this->pending.emplace_back(n);
    this->current = std::make_pair(n->pt, n->value);
  }
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Iterator&
KDTree<CoordType, dimensions, ValueType>::Iterator::operator++() {
  const Node* n = this->pending.front();
  this->pending.pop_front();

  if (n->before) {
    this->pending.emplace_back(n->before);
  }
  if (n->after) {
    this->pending.emplace_back(n->after);
  }

  if (!this->pending.empty()) {
    n = this->pending.front();
    this->current = std::make_pair(n->pt, n->value);
  }

  return *this;
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Iterator
KDTree<CoordType, dimensions, ValueType>::Iterator::operator++(int) {
  Iterator ret = *this;
  this->operator++();
  return ret;
}

template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::Iterator::operator==(
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

template <typename CoordType, size_t dimensions, typename ValueType>
bool KDTree<CoordType, dimensions, ValueType>::Iterator::operator!=(
    const Iterator& other) const {
  return !this->operator==(other);
}

template <typename CoordType, size_t dimensions, typename ValueType>
const typename std::pair<typename KDTree<CoordType, dimensions, ValueType>::Point, ValueType>&
KDTree<CoordType, dimensions, ValueType>::Iterator::operator*() const {
  return this->current;
}

template <typename CoordType, size_t dimensions, typename ValueType>
const typename std::pair<typename KDTree<CoordType, dimensions, ValueType>::Point, ValueType>*
KDTree<CoordType, dimensions, ValueType>::Iterator::operator->() const {
  return &this->current;
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Iterator
KDTree<CoordType, dimensions, ValueType>::begin() const {
  return Iterator(this->root);
}

template <typename CoordType, size_t dimensions, typename ValueType>
typename KDTree<CoordType, dimensions, ValueType>::Iterator
KDTree<CoordType, dimensions, ValueType>::end() const {
  return Iterator(NULL);
}
