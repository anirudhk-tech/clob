#include "clob/book.hpp"
#include "clob/price_level.hpp"
#include "clob/order.hpp"

namespace clob {

constexpr PriceTicks MIN_PRICE_TICKS = 0;
constexpr PriceTicks MAX_PRICE_TICKS = 1000000;

Book::Book(std::size_t max_orders)
  : pool_(max_orders)
  , id_map_(max_orders)
{
  bid_level_ = new PriceLevel{};
  bid_level_->price_ticks = MIN_PRICE_TICKS;
  ask_level_ = new PriceLevel();
  ask_level_->price_ticks = MAX_PRICE_TICKS;
}

auto Book::is_valid_price(PriceTicks price) noexcept
{
  return MIN_PRICE_TICKS <= price && price <= MAX_PRICE_TICKS;
}

Book::AddResult Book::add_limit(OrderId order_id, Qty qty, Side side, PriceTicks price) 
{
  if (qty <= 0) {
    return {.accepted = false, .reject_reason = "qty <= 0"};
  }

  if (!is_valid_price(price)) {
    return {.accepted = false, .reject_reason = "invalid price"};
  }

  if (id_map_.exists(order_id)) {
    return {.accepted = false, .reject_reason = "duplicate order_id"};
  }

  Order* order = pool_.allocate();
  if (order == nullptr) {
    return {.accepted = false, .reject_reason = "pool full"};
  }

  order->order_id = order_id;
  order->side = side;
  order->price_ticks = price;
  order->qty_remaining = qty;
  assign_time_seq(*order);

  id_map_.set(order_id, order);

  PriceLevel* level = (side == Side::Buy) ? bid_level_ : ask_level_;
  level->push_back(order);

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

  PriceLevel* level = (order->side == Side::Buy) ? bid_level_ : ask_level_;
  level->erase(order);

  id_map_.clear(order_id);
  pool_.free(order);

  return true;
}

}
