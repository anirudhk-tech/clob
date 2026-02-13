#include "clob/order.hpp"

#include <cassert>

namespace clob {

OrderPool::OrderPool(std::size_t capacity)
  : storage_(capacity)
{
  for (auto node : storage_) {
    node.prev = nullptr;
    node.next = free_head_;
    free_head_ = &node;
    ++free_count_;
  }

  assert(free_count_ == storage_.size());
}
  
}

