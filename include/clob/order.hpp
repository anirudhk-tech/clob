#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "clob/types.hpp"

namespace clob {

enum class Side : std::uint8_t {
  Buy,
  Sell
};

struct Order {
  OrderId    order_id{};
  Side       side{Side::Buy};
  PriceTicks price_ticks{};
  Qty        qty_remaining{};

  std::uint64_t time_seq{};
  Order* prev{nullptr};
  Order* next{nullptr};
  
  [[nodiscard]] auto is_live() const noexcept {
    return qty_remaining > 0;
  }
};

class OrderPool {
public:
  explicit OrderPool(std::size_t capacity);

  auto allocate();
  void free(Order* order);

  [[nodiscard]] auto capacity() const noexcept;
  [[nodiscard]] auto free_count() const noexcept;

private:
  std::vector<Order> storage_;
  Order* free_head_{nullptr};

  std::size_t free_count_{0};
};

class OrderIdMap {
public:
  explicit OrderIdMap(std::size_t max_orders);

  [[nodiscard]] auto get(OrderId order_id) const noexcept;

  void set(OrderId order_id, Order* order) noexcept;

  void clear(OrderId order_id) noexcept;

  [[nodiscard]] auto exists(OrderId order_id) const noexcept;

  [[nodiscard]] auto max_id() const noexcept;

private:
  std::vector<Order*> by_id_;
};

} // namespace clob
