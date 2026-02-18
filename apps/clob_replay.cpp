#include "clob/book.hpp"
#include "clob/types.hpp"

#include <cstdint>
#include <cstddef>
#include <iostream>

using namespace clob;

static inline std::uint64_t fnv1a64_mix(std::uint64_t h, const void* data, std::size_t n) {
  const auto* p = static_cast<const std::uint8_t*>(data);
  for (std::size_t i = 0; i < n; ++i) {
    h ^= std::uint64_t(p[i]);
    h *= 1099511628211ull;
  }
  return h;
}

template <typename T>
static inline void hash_add(std::uint64_t& h, const T& v) {
  h = fnv1a64_mix(h, &v, sizeof(T));
}

static inline void hash_add_sv(std::uint64_t& h, std::string_view sv) {
  std::uint64_t n = static_cast<std::uint64_t>(sv.size());
  hash_add(h, n);
  h = fnv1a64_mix(h, sv.data(), sv.size());
}

struct HashSink final : Book::EventSink {
  std::uint64_t h = 14695981039346656037ull;
  std::uint64_t count = 0;

  void on_ack_add(const Book::AckAddEvent& e) override{
    std::uint8_t tag = 1;
    hash_add(h, tag);
    hash_add(h, e.order_id);
    ++count;
  }

  void on_reject_add(const Book::RejectAddEvent& e) override{
    std::uint8_t tag = 2;
    hash_add(h, tag);
    hash_add(h, e.order_id);
    hash_add_sv(h, e.reason);
    ++count;
  }

  void on_ack_cancel(const Book::AckCancelEvent& e) override {
    std::uint8_t tag = 3;
    hash_add(h, tag);
    hash_add(h, e.order_id);
    ++count;
  }

  void on_reject_cancel(const Book::RejectCancelEvent& e) override {
    std::uint8_t tag = 4;
    hash_add(h, tag);
    hash_add(h, e.order_id);
    hash_add_sv(h, e.reason);
    ++count;
  }

  void on_trade(const Book::TradeEvent& e) override {
    std::uint8_t tag = 5;
    hash_add(h, tag);
    hash_add(h, e.resting_id);
    hash_add(h, e.incoming_id);
    hash_add(h, e.price);
    hash_add(h, e.qty);
    ++count;
  }

  void on_done(const Book::DoneEvent& e) override {
    std::uint8_t tag = 6;
    hash_add(h, tag);
    hash_add(h, e.order_id);
    ++count;
  }
};

int main() {
  Book book(1'000'000);
  HashSink sink;
  book.set_sink(&sink);

  book.add_limit(1, 10, Side::Sell, 101);
  book.add_limit(2, 10, Side::Sell, 101);
  book.add_limit(3, 10, Side::Buy,  99);
  book.add_limit(4, 5,  Side::Buy,  101);

  book.cancel(3);
  book.cancel(999999);

  book.add_limit(1, 1, Side::Buy, 200);

  book.add_limit(5, 20, Side::Buy,  1000);
  book.add_limit(6, 20, Side::Sell, 1000);

  std::cout << "hash=" << sink.h << " events=" << sink.count << "\n";
  return 0;
}

