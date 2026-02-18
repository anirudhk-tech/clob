#include "clob/ladder.hpp"
#include "clob/order.hpp"
#include "clob/price_level.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace clob {

class Book {
public:
  explicit Book(std::size_t max_orders);

  struct AddResult {
    bool accepted;
    std::optional<std::string_view> reject_reason;
  };

  struct TradeEvent { OrderId resting_id; OrderId incoming_id; PriceTicks price; Qty qty; };
  struct DoneEvent  { OrderId order_id; };
  struct EventSink {
    void on_trade(const TradeEvent&) {}
    void on_done(const DoneEvent&) {}
  };
  
  AddResult add_limit(OrderId order_id, Qty qty, Side side, PriceTicks price);

  auto cancel(OrderId order_id) noexcept;

private:
  OrderPool pool_;
  OrderIdMap id_map_;
  Ladder ladder_;

  PriceLevel* bid_level_{nullptr};
  PriceLevel* ask_level_{nullptr};

  std::uint64_t next_time_seq_{1};

  [[nodiscard]] static auto is_valid_price(PriceTicks price) noexcept;
  void assign_time_seq(Order& order) noexcept;
};

}
