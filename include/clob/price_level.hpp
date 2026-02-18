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

  PriceLevel* bid_prev{nullptr};
  PriceLevel* bid_next{nullptr};
  PriceLevel* ask_prev{nullptr};
  PriceLevel* ask_next{nullptr};

  bool in_bid{false};
  bool in_ask{false};

  void push_back(Order* order) noexcept;
  Order* pop_front() noexcept;
  void erase(Order* order) noexcept;
  [[nodiscard]] auto empty() const noexcept { return head == nullptr; };
};

}

