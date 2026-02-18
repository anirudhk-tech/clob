#include "clob/ladder.hpp"
#include "clob/types.hpp"

#include <cassert>
#include <cstddef>

namespace clob {

Ladder::Ladder(LadderConfig cfg)
  : cfg_(cfg),
    levels_(static_cast<std::size_t>(cfg.max_price_ticks - cfg.min_price_ticks + 1)) {
      for (PriceTicks p = cfg_.min_price_ticks; p <= cfg_.max_price_ticks; ++p) {
        levels_[index_of(p)].price_ticks = p;
      }
    }

bool Ladder::is_valid_price(PriceTicks p) const noexcept {
  return cfg_.min_price_ticks <= p && p <= cfg_.max_price_ticks;
}

PriceTicks Ladder::min_price_ticks() const noexcept { return cfg_.min_price_ticks; };
PriceTicks Ladder::max_price_ticks() const noexcept { return cfg_.max_price_ticks; };

std::size_t Ladder::index_of(PriceTicks p) const noexcept {
  return static_cast<std::size_t>(p - cfg_.min_price_ticks);
}

PriceLevel& Ladder::level_at(PriceTicks p) noexcept {
  assert(is_valid_price(p));
  return levels_[index_of(p)];
}

PriceLevel* Ladder::best_bid_level() const noexcept { return best_bid_; }
PriceLevel* Ladder::best_ask_level() const noexcept { return best_ask_; }

void Ladder::on_bid_level_became_non_empty(PriceLevel& lvl) noexcept {
  assert(!lvl.empty());
  if (lvl.in_bid) return;
  bid_insert_sorted(lvl);
}

void Ladder::on_bid_level_became_empty(PriceLevel& lvl) noexcept {
  assert(lvl.empty());
  if (!lvl.in_bid) return;
  bid_erase(lvl);
}

void Ladder::on_ask_level_became_non_empty(PriceLevel& lvl) noexcept {
  assert(!lvl.empty());
  if (lvl.in_ask) return;
  ask_insert_sorted(lvl);
}

void Ladder::on_ask_level_became_empty(PriceLevel& lvl) noexcept {
  assert(lvl.empty());
  if (!lvl.in_ask) return;
  ask_erase(lvl);
}

void Ladder::bid_insert_sorted(PriceLevel& lvl) noexcept {
  lvl.in_bid = true;
  lvl.bid_prev = nullptr;
  lvl.bid_next = nullptr;

  if (!best_bid_) {
      best_bid_ = &lvl;
      return;
  }

  if (lvl.price_ticks > best_bid_->price_ticks) {
    lvl.bid_next = best_bid_;
    best_bid_->bid_prev = &lvl;
    best_bid_ = &lvl;
    return;
  }
  
  PriceLevel* cur = best_bid_;
  while (cur->bid_next && cur->bid_next->price_ticks >= lvl.price_ticks) {
    cur = cur->bid_next;
  }

  lvl.bid_next = cur->bid_next;
  lvl.bid_prev = cur;
  
  if (cur->bid_next) cur->bid_next->bid_prev = &lvl;
  cur->bid_next = &lvl;
}

void Ladder::ask_insert_sorted(PriceLevel& lvl) noexcept {
  lvl.in_ask = true;
  lvl.ask_prev = nullptr;
  lvl.ask_next = nullptr;

  if (!best_ask_) {
    best_ask_ = &lvl;
    return;
  }

  if (lvl.price_ticks < best_ask_->price_ticks) {
    lvl.ask_next = best_ask_;
    best_ask_->ask_prev = &lvl;
    best_ask_ = &lvl;
    return;
  }

  PriceLevel* cur = best_ask_;
  while (cur->ask_next && cur->ask_next->price_ticks <= lvl.price_ticks) {
    cur = cur->ask_next;
  }

  lvl.ask_next = cur->ask_next;
  lvl.ask_prev = cur;

  if (cur->ask_next) cur->ask_next->ask_prev = &lvl;
  cur->ask_next = &lvl;
}

void Ladder::bid_erase(PriceLevel& lvl) noexcept {
  if (lvl.bid_prev) lvl.bid_prev->bid_next = lvl.bid_next;
  else best_bid_ = lvl.bid_next;

  if (lvl.bid_next) lvl.bid_next->bid_prev = lvl.bid_prev;

  lvl.bid_prev = nullptr;
  lvl.bid_next = nullptr;
  lvl.in_bid = false;
}

void Ladder::ask_erase(PriceLevel& lvl) noexcept {
  if (lvl.ask_prev) lvl.ask_prev->ask_next = lvl.ask_next;
  else best_ask_ = lvl.ask_next;

  if (lvl.ask_next) lvl.ask_next->ask_prev = lvl.ask_prev;

  lvl.ask_prev = nullptr;
  lvl.ask_next = nullptr;
  lvl.in_ask = false;
}

} // namespace clob
