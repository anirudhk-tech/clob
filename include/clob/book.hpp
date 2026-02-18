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
  struct AckAddEvent { OrderId order_id; };
  struct RejectAddEvent { OrderId order_id; std::string_view reason; };
  struct AckCancelEvent { OrderId order_id; };
  struct RejectCancelEvent { OrderId order_id; std::string_view reason; };

  struct EventSink {
    virtual ~EventSink() = default;
    virtual void on_ack_add(const AckAddEvent&) {}
    virtual void on_reject_add(const RejectAddEvent&) {}
    virtual void on_ack_cancel(const AckCancelEvent&) {}
    virtual void on_reject_cancel(const RejectCancelEvent&) {}
    virtual void on_trade(const TradeEvent&) {}
    virtual void on_done(const DoneEvent&) {} 
  };

  void set_sink(EventSink* sink) noexcept { sink_ = sink; };

  void match_buy (OrderId incoming_id, PriceTicks limit_price, Qty& incoming_qty);
  void match_sell(OrderId incoming_id, PriceTicks limit_price, Qty& incoming_qty);
  
  AddResult add_limit(OrderId order_id, Qty qty, Side side, PriceTicks price);

  bool cancel(OrderId order_id) noexcept;

private:
  OrderPool pool_;
  OrderIdMap id_map_;
  Ladder ladder_;

  EventSink* sink_{nullptr};

  PriceLevel* bid_level_{nullptr};
  PriceLevel* ask_level_{nullptr};

  std::uint64_t next_time_seq_{1};

  [[nodiscard]] static auto is_valid_price(PriceTicks price) noexcept;
  void assign_time_seq(Order& order) noexcept;
};

}
