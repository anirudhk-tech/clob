#include "clob/price_level.hpp"
#include "clob/order.hpp"

#include <cassert>

namespace clob {

void PriceLevel::push_back(Order* order) noexcept {
  assert(order);
  assert(order->prev == nullptr);
  assert(order->next == nullptr);

  order->prev = tail;
  order->next = nullptr;

  if (tail) {
    tail->next = order;
  } else {
    head = order;
  }
  tail = order;
}

auto PriceLevel::pop_front() noexcept {
  if (!head) return static_cast<Order*>(nullptr);

  Order* order = head;
  head = order->next;

  if (head) {
    head->prev = nullptr;
  } else {
    tail = nullptr;
  }

  order->prev = nullptr;
  order->next = nullptr;
  return order;
}

void PriceLevel::erase(Order* order) noexcept {
  assert(order);

  if (order->prev) {
    order->prev->next = order->next;
  } else {
    head = order->next;
  }

  if (order->next) {
    order->next->prev = order->prev;
  } else {
    tail = order->prev;
  }

  order->prev = nullptr;
  order->next = nullptr;
}

} // namespace clob

