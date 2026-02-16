#include "clob/order.hpp"
#include "clob/price_level.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace clob {

class Book {
public:
  explicit Book(std::size_t max_orders);

  struct AddResult {
    bool accepted;
    std::optional<std::string_view> reject_reason;
  };
  auto add_limit(OrderId order_id, Side side, PriceTicks price, Qty qty);

  auto cancel(OrderId order_id);

  // debug
  [[nodiscard]] auto num_live_order() const noexcept;
  [[nodiscard]] auto best_bid() const noexcept;
  [[nodiscard]] auto best_ask() const noexcept;

private:
  OrderPool pool_;
  OrderIdMap id_map_;

  PriceLevel* bid_level_{nullptr};
  PriceLevel* ask_level_{nullptr};

  std::uint64_t next_time_seq_{1};

  auto is_valid_price(PriceTicks price) noexcept;
  void assign_time_seq(Order& order) noexcept;
};

}
