#pragma once

#include "clob/price_level.hpp"
#include "clob/types.hpp"

#include <cstddef>
#include <vector>

namespace clob {

struct LadderConfig {
  PriceTicks min_price_ticks{0};
  PriceTicks max_price_ticks{1'000'000};
};

class Ladder {
public:
  explicit Ladder(LadderConfig cfg);

  [[nodiscard]] bool is_valid_price(PriceTicks p) const noexcept;
  [[nodiscard]] PriceTicks min_price_ticks() const noexcept;
  [[nodiscard]] PriceTicks max_price_ticks() const noexcept;

  PriceLevel& level_at(PriceTicks p) noexcept;
  const PriceLevel& level_at(PriceTicks p) const noexcept;

  void on_bid_level_became_non_empty(PriceLevel& lvl) noexcept;
  void on_bid_level_became_empty(PriceLevel& lvl) noexcept;

  void on_ask_level_became_non_empty(PriceLevel& lvl) noexcept;
  void on_ask_level_became_empty(PriceLevel& lvl) noexcept;

  [[nodiscard]] PriceLevel* best_bid_level() const noexcept;
  [[nodiscard]] PriceLevel* best_ask_level() const noexcept;

private:
  LadderConfig cfg_;
  std::vector<PriceLevel> levels_;

  PriceLevel* best_bid_{nullptr};
  PriceLevel* best_ask_{nullptr};

  [[nodiscard]] std::size_t index_of(PriceTicks p) const noexcept;

  void bid_insert_sorted(PriceLevel& lvl) noexcept;
  void ask_insert_sorted(PriceLevel& lvl) noexcept;

  void bid_erase(PriceLevel& lvl) noexcept;
  void ask_erase(PriceLevel& lvl) noexcept;
};

} // namespace clob
