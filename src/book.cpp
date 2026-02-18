#include "clob/book.hpp"
#include "clob/price_level.hpp"
#include "clob/order.hpp"

namespace clob {

Book::Book(std::size_t max_orders)
  : pool_(max_orders)
  , id_map_(max_orders)
  , ladder_(LadderConfig())
{

}

Book::AddResult Book::add_limit(OrderId order_id, Qty qty, Side side, PriceTicks price) 
{
  if (qty <= 0) {
    return {.accepted = false, .reject_reason = "qty <= 0"};
  }

  if (!ladder_.is_valid_price(price)) {
    return {.accepted = false, .reject_reason = "invalid price"};
  }

  if (id_map_.exists(order_id)) {
    return {.accepted = false, .reject_reason = "duplicate order_id"};
  }

 Qty incoming_qty = qty;

  if (side == Side::Buy) match_buy(id, price, incoming_qty);
  else                  match_sell(id, price, incoming_qty);

  if (incoming_qty == 0) {
    return {.accepted = true, .reject_reason = {}};
  }

  Order* inc = pool_.allocate();
  if (!inc) return {.accepted = false, .reject_reason = "pool full"};

  inc->order_id = order_id;
  inc->side = side;
  inc->price_ticks = price;
  inc->qty_remaining = incoming_qty;
  inc->prev = nullptr;
  inc->next = nullptr;
  assign_time_seq(*inc);

  id_map_.set(order_id, inc);

  PriceLevel& lvl = ladder_.level_at(price);
  bool was_empty = lvl.empty();
  lvl.push_back(inc);
  if (was_empty) {
    if (side == Side::Buy) ladder_.on_bid_level_became_non_empty(lvl);
    else                  ladder_.on_ask_level_became_non_empty(lvl);
  }

  return {.accepted = true, .reject_reason = {}};
}

void Book::assign_time_seq(Order& order) noexcept
{
  order.time_seq = next_time_seq_++;
}

auto Book::cancel(OrderId order_id) noexcept
{
  Order* order = id_map_.get(order_id);
  if (order == nullptr) {
    return false;
  }

  PriceLevel& lvl = ladder_.level_at(order->price_ticks);
  lvl.erase(order);
  if (lvl.empty()) {
    if (order->side == Side::Buy) ladder_.on_bid_level_became_empty(lvl);
    else ladder_.on_ask_level_became_empty(lvl);
  } 

  id_map_.clear(order_id);
  pool_.free(order);

  return true;
}

static inline Qty min_qty(Qty a, Qty b) { return (a < b) ? a : b; }

void Book::match_buy(OrderId incoming_id, PriceTicks limit_price, Qty& incoming_qty) {
  while (incoming_qty > 0) {
    PriceLevel* lvl = ladder_.best_ask_level();
    if (!lvl) break;
    if (lvl->price_ticks > limit_price) break;

    while (incoming_qty > 0 && !lvl->empty()) {
      Order* rest = lvl->head;
      Qty t = min_qty(incoming_qty, rest->qty_remaining);

      incoming_qty -= t;
      rest->qty_remaining -= t;
      
      if (rest->qty_remaining == 0) {
        lvl->pop_front();
        id_map_.clear(rest->order_id);
        pool_.free(rest);
      }

      if (lvl->empty()) {
        ladder_.on_ask_level_became_empty(*lvl);
      }
    }
  }
}

void Book::match_sell(OrderId incoming_id, PriceTicks limit_price, Qty& incoming_qty) {
  while (incoming_qty > 0) {
    PriceLevel* lvl = ladder_.best_bid_level();
    if (!lvl) break;
    if (lvl->price_ticks < limit_price) break;

    while (incoming_qty > 0 && !lvl->empty()) {
      Order* rest = lvl->head;
      Qty t = min_qty(incoming_qty, rest->qty_remaining);

      incoming_qty -= t;
      rest->qty_remaining -= t;

      if (rest->qty_remaining == 0) {
        lvl->pop_front();
        id_map_.clear(rest->order_id);
        pool_.free(rest);
      }
    }

    if (lvl->empty()) {
      ladder_.on_bid_level_became_empty(*lvl);
    }
  }
}

}
