#pragma once

#include "clob/types.hpp"

namespace clob {

struct Order;

}

namespace clob {

struct PriceLevel {
  PriceTicks price_ticks{};

  Order* head{nullptr};
  Order* tail{nullptr};

  void push_back(Order* order) noexcept;
  auto pop_front() noexcept;
  void erase(Order* order) noexcept;
  [[nodiscard]] auto empty() const noexcept { return head == nullptr; };
};

}

