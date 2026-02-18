#include "clob/order.hpp"

#include <cassert>
#include <cstddef>

namespace clob {

OrderPool::OrderPool(std::size_t capacity)
  : storage_(capacity)
{
  for (auto& node : storage_) {
    node.prev = nullptr;
    node.next = free_head_;
    free_head_ = &node;
    ++free_count_;
  }

  assert(free_count_ == storage_.size());
}

Order* OrderPool::allocate()
{
  if (free_head_ == nullptr) {
    return static_cast<Order*>(nullptr);
  }

  Order* node = free_head_;

  node->prev = nullptr;
  node->next = nullptr;

  free_head_ = node->next;
  node->next = nullptr;
  node->order_id = 0;
  node->qty_remaining = 0;
  node->time_seq = 0;

  assert(free_count_ > 0);
  --free_count_;

  return node;
}

void OrderPool::free(Order* order)
{
  if (order == nullptr) {
    return;
  }

  assert(order->prev == nullptr && order->next == nullptr);

  order->next = free_head_;
  order->prev = nullptr;

  free_head_ = order;
  ++free_count_;
}

auto OrderPool::capacity() const noexcept 
{
  return storage_.size();
}

auto OrderPool::free_count() const noexcept
{
  return free_count_;
}

OrderIdMap::OrderIdMap(std::size_t max_orders)
  : by_id_(max_orders + 1, nullptr)
{

}

Order* OrderIdMap::get(OrderId order_id) const noexcept 
{
  if (order_id >= by_id_.size()) {
    return static_cast<Order*>(nullptr);
  }
  return by_id_[order_id];
}

void OrderIdMap::set(OrderId order_id, Order* order) noexcept
{
  assert(order_id > 0 && order_id < by_id_.size());
  by_id_[order_id] = order;
}

void OrderIdMap::clear(OrderId order_id) noexcept
{
  if (order_id == 0 || order_id >= by_id_.size()) {
    return;
  }

  by_id_[order_id] = nullptr;
}

bool OrderIdMap::exists(OrderId order_id) const noexcept
{
  return by_id_[order_id] != nullptr;
}

auto OrderIdMap::max_id() const noexcept
{
  return by_id_.size() - 1;
}

}



