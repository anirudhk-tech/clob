# clob

A lightweight in-memory limit order book with price-time priority, event-sink callbacks, and allocation-free hot path (order pool, fixed ladder).

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Performance](#performance)
- [Design](#design)
- [Limitations](#limitations)
- [Platform Support](#platform-support)
- [License](#license)

## Overview

clob is a single-instrument limit order book: add limit orders, cancel by ID, and match incoming orders against the book (buy vs ask, sell vs bid). Resting orders are stored in a price ladder with time priority within each level. An optional **event sink** receives acks, rejects, trades, and done events so you can drive downstream logic (logging, risk, matching engine output) without polling.

This is aimed at:

- **Matching engines** — core book logic with callback-driven events
- **Backtesting** — replay order streams and observe trades/done events
- **Low-latency** — no heap allocation on add/cancel/match; pool and ladder are preallocated

## Features

- **Price-time priority** — best bid/ask maintained; FIFO within each price level
- **Event sink** — `EventSink` callbacks for ack_add, reject_add, ack_cancel, reject_cancel, trade, done
- **Allocation-free hot path** — `OrderPool` and `OrderIdMap` fixed at construction; no `new`/`delete` during matching
- **Zero dependencies** — C++20, standard library only
- **Modern CMake** — sanitizer options (ASAN, UBSAN), compile commands export

## Installation

### CMake (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
  clob
  GIT_REPOSITORY https://github.com/youruser/clob.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(clob)

target_link_libraries(your_target PRIVATE clob)
```

### Manual

```bash
git clone https://github.com/youruser/clob.git
cd clob
cmake -S . -B build
cmake --build build -j
```

Link against `clob` and add `include/` to your include path.

## Quick Start

```cpp
#include "clob/book.hpp"
#include "clob/types.hpp"
#include <iostream>

int main() {
  clob::Book book(100'000);

  struct MySink : clob::Book::EventSink {
    void on_ack_add(const clob::Book::AckAddEvent& e) override {
      std::cout << "ack add " << e.order_id << "\n";
    }
    void on_trade(const clob::Book::TradeEvent& e) override {
      std::cout << "trade resting=" << e.resting_id << " incoming=" << e.incoming_id
                << " price=" << e.price << " qty=" << e.qty << "\n";
    }
    void on_done(const clob::Book::DoneEvent&) override {}
  } sink;
  book.set_sink(&sink);

  auto r = book.add_limit(1, 10, clob::Side::Sell, 100);
  r = book.add_limit(2, 5, clob::Side::Buy, 100);  // matches 5
  book.cancel(1);
}
```

## API Reference

### Types

| Type        | Definition     | Description              |
|-------------|----------------|--------------------------|
| `OrderId`   | `std::uint32_t`| Unique order identifier  |
| `PriceTicks`| `std::int32_t` | Price in ticks           |
| `Qty`       | `std::int64_t` | Quantity                 |
| `Side`      | enum           | `Side::Buy`, `Side::Sell`|

### Book

```cpp
namespace clob {
  class Book {
  public:
    explicit Book(std::size_t max_orders);

    struct AddResult { bool accepted; std::optional<std::string_view> reject_reason; };
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

    void set_sink(EventSink* sink) noexcept;

    AddResult add_limit(OrderId order_id, Qty qty, Side side, PriceTicks price);
    bool cancel(OrderId order_id) noexcept;
  };
}
```

- **add_limit** — Adds a limit order; matches immediately against the opposite side (buy vs best ask, sell vs best bid), then any remainder rests in the book. Returns `AddResult`; on reject, optional reason and `EventSink::on_reject_add` if set.
- **cancel** — Removes the order by ID. Returns `false` if unknown order; otherwise `true` and `on_ack_cancel` if set.
- **set_sink** — Optional. Pass `nullptr` to disable callbacks.

### Ladder and price range

`Ladder` is configured with `LadderConfig{min_price_ticks, max_price_ticks}` (defaults in the implementation). Orders outside this range are rejected with "invalid price".

## Performance

The included benchmark (`book_bench`) exercises add-only (resting), cancel-only, marketable match (incoming always crosses), and a mixed stream (adds + cancels + marketable). Example output:

```
add_resting ops=2000000 sec=0.0176006 ns_per_op=8.80031 ops_per_s=1.13632e+08
cancel ops=1000000 sec=0.00158404 ns_per_op=1.58404 ops_per_s=6.31297e+08
marketable_match ops=2000000 sec=0.00763629 ns_per_op=3.81815 ops_per_s=2.61907e+08
mixed_stream ops=2500000 sec=0.0174533 ns_per_op=6.98132 ops_per_s=1.43239e+08
process_total_new_calls=14
```

The process allocation count (`process_total_new_calls=14`) confirms no heap allocation during the timed loops (only startup/book construction). Build with release settings for meaningful numbers.

Run the benchmark:

```bash
./build/book_bench
```

### Building

```bash
# Standard build
cmake -S . -B build
cmake --build build -j

# With AddressSanitizer
cmake -S . -B build-asan -DCLOB_ASAN=ON
cmake --build build-asan -j

# With UndefinedBehaviorSanitizer
cmake -S . -B build-ubsan -DCLOB_UBSAN=ON
cmake --build build-ubsan -j
```

The CMake configuration uses `-Wall -Wextra -Wpedantic -Werror` and C++20 by default.

## Design

- **Order pool** — Fixed-capacity pool of `Order` nodes; `allocate()`/`free()` no-throw, no heap.
- **Order ID map** — Direct index by `OrderId` up to `max_orders` for O(1) lookup and cancel.
- **Ladder** — Contiguous price levels (vector); each level is a doubly-linked list of orders (time order). Best bid/ask maintained via pointers; levels linked in price order for bid and ask.
- **Matching** — Incoming buy (sell) walks best ask (bid) and matches until quantity exhausted or price no longer crossing; filled resting orders are removed and freed; remainder is added to the book.
- **Event sink** — Optional; callbacks invoked synchronously from `add_limit` and `cancel` (e.g. on_ack_add, on_trade, on_done, on_ack_cancel).

## Limitations

| Limitation        | Explanation |
|-------------------|-------------|
| **Single instrument** | One book instance = one symbol; no multi-venue or multi-symbol in this library |
| **No partial cancel** | Cancel is full order only (by ID) |
| **No amend**        | No replace/amend; cancel + add_limit to change price or size |
| **Single-threaded** | No internal locking; synchronize externally if used from multiple threads |
| **Price in ticks** | No built-in decimal conversion; use your own tick-to-price mapping |

## Platform Support

| Platform | Status |
|----------|--------|
| macOS    | Supported (Clang) |
| Linux    | Supported (GCC/Clang) |
| Windows  | Supported (MSVC; CMake uses `/W4` etc.) |

Requirements: C++20, CMake 3.20+.

## License

MIT License. See LICENSE for details.

---

**clob** — Fast, allocation-free limit order book with price-time priority and event-sink callbacks.
