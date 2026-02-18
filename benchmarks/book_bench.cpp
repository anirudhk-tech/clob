#include "clob/book.hpp"
#include "clob/types.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

using namespace clob;

static inline std::uint64_t ns_now() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

static inline std::uint32_t lcg(std::uint32_t& s) {
  s = 1664525u * s + 1013904223u;
  return s;
}

template <class T>
static inline void do_not_optimize(T const& value) {
#if defined(__clang__) || defined(__GNUC__)
  asm volatile("" : : "g"(value) : "memory");
#else
  (void)value;
#endif
}

static std::atomic<std::uint64_t> g_new_calls{0};

void* operator new(std::size_t n) {
  g_new_calls.fetch_add(1, std::memory_order_relaxed);
  if (void* p = std::malloc(n)) return p;
  std::abort();
}
void operator delete(void* p) noexcept { std::free(p); }

void* operator new[](std::size_t n) {
  g_new_calls.fetch_add(1, std::memory_order_relaxed);
  if (void* p = std::malloc(n)) return p;
  std::abort();
}
void operator delete[](void* p) noexcept { std::free(p); }

void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

static void report(const char* name, std::size_t ops, std::uint64_t ns) {
  const double sec = double(ns) * 1e-9;
  const double ops_per_s = sec > 0.0 ? (double(ops) / sec) : 0.0;
  const double ns_per_op = ops ? (double(ns) / double(ops)) : 0.0;
  std::cout << name
            << " ops=" << ops
            << " sec=" << sec
            << " ns_per_op=" << ns_per_op
            << " ops_per_s=" << ops_per_s
            << "\n";
}

static void check_allocs(const char* name, std::uint64_t before, std::uint64_t after) {
  const std::uint64_t delta = after - before;
  if (delta != 0) {
    std::cerr << name << " ERROR: allocations during timed loop = " << delta << "\n";
  }
}

static void bench_add_resting(std::size_t max_orders,
                              std::size_t warmup_ops,
                              std::size_t ops,
                              OrderId start_id) {
  Book book(max_orders);
  std::uint32_t rng = 1;
  OrderId id = start_id;

  for (std::size_t i = 0; i < warmup_ops; ++i) {
    const std::uint32_t r = lcg(rng);
    const Side side = (r & 1u) ? Side::Buy : Side::Sell;
    const PriceTicks price = static_cast<PriceTicks>(10000 + (r % 100));
    const Qty qty = static_cast<Qty>(1 + (r % 10));
    const auto res = book.add_limit(id++, qty, side, price);
    do_not_optimize(res.accepted);
  }

  const std::uint64_t new_before = g_new_calls.load(std::memory_order_relaxed);

  const std::uint64_t t0 = ns_now();
  for (std::size_t i = 0; i < ops; ++i) {
    const std::uint32_t r = lcg(rng);
    const Side side = (r & 1u) ? Side::Buy : Side::Sell;
    const PriceTicks price = static_cast<PriceTicks>(10000 + (r % 100));
    const Qty qty = static_cast<Qty>(1 + (r % 10));
    const auto res = book.add_limit(id++, qty, side, price);
    do_not_optimize(res.accepted);
  }
  const std::uint64_t t1 = ns_now();

  const std::uint64_t new_after = g_new_calls.load(std::memory_order_relaxed);

  report("add_resting", ops, (t1 - t0));
  check_allocs("add_resting", new_before, new_after);
}

static void bench_cancel(std::size_t max_orders,
                         std::size_t warmup_ops,
                         std::size_t ops,
                         OrderId start_id) {
  Book book(max_orders);

  std::uint32_t rng = 2;
  OrderId id = start_id;

  std::vector<OrderId> live;
  live.reserve(ops);

  for (std::size_t i = 0; i < ops; ++i) {
    const std::uint32_t r = lcg(rng);
    const Side side = (r & 1u) ? Side::Buy : Side::Sell;
    const PriceTicks price = static_cast<PriceTicks>(20000 + (r % 100));
    const Qty qty = static_cast<Qty>(1 + (r % 10));
    const auto res = book.add_limit(id, qty, side, price);
    do_not_optimize(res.accepted);
    live.push_back(id);
    ++id;
  }

  for (std::size_t i = 0; i < warmup_ops && i < live.size(); ++i) {
    const bool ok = book.cancel(live[i]);
    do_not_optimize(ok);
  }
  OrderId warm_id = id;
  for (std::size_t i = 0; i < warmup_ops && i < live.size(); ++i) {
    const std::uint32_t r = lcg(rng);
    const Side side = (r & 1u) ? Side::Buy : Side::Sell;
    const PriceTicks price = static_cast<PriceTicks>(20000 + (r % 100));
    const Qty qty = static_cast<Qty>(1 + (r % 10));
    const auto res = book.add_limit(warm_id, qty, side, price);
    do_not_optimize(res.accepted);
    live[i] = warm_id;
    ++warm_id;
  }

  const std::uint64_t new_before = g_new_calls.load(std::memory_order_relaxed);

  const std::uint64_t t0 = ns_now();
  for (std::size_t i = 0; i < ops; ++i) {
    const bool ok = book.cancel(live[i]);
    do_not_optimize(ok);
  }
  const std::uint64_t t1 = ns_now();

  const std::uint64_t new_after = g_new_calls.load(std::memory_order_relaxed);

  report("cancel", ops, (t1 - t0));
  check_allocs("cancel", new_before, new_after);
}

static void bench_marketable_match(std::size_t max_orders,
                                   std::size_t warmup_ops,
                                   std::size_t ops,
                                   OrderId start_id) {
  Book book(max_orders);
  OrderId id = start_id;

  for (int i = 0; i < 1000; ++i) {
    const auto res = book.add_limit(id++, 1'000'000, Side::Sell, 10000);
    do_not_optimize(res.accepted);
  }

  for (std::size_t i = 0; i < warmup_ops; ++i) {
    const auto res = book.add_limit(id++, 1, Side::Buy, 20000);
    do_not_optimize(res.accepted);
  }

  const std::uint64_t new_before = g_new_calls.load(std::memory_order_relaxed);

  const std::uint64_t t0 = ns_now();
  for (std::size_t i = 0; i < ops; ++i) {
    const auto res = book.add_limit(id++, 1, Side::Buy, 20000);
    do_not_optimize(res.accepted);
  }
  const std::uint64_t t1 = ns_now();

  const std::uint64_t new_after = g_new_calls.load(std::memory_order_relaxed);

  report("marketable_match", ops, (t1 - t0));
  check_allocs("marketable_match", new_before, new_after);
}

static void bench_mixed_stream(std::size_t max_orders,
                               std::size_t warmup_iters,
                               std::size_t iters,
                               OrderId start_id) {
  Book book(max_orders);

  std::uint32_t rng = 42;
  OrderId id = start_id;

  std::vector<OrderId> cancellable;
  cancellable.reserve(iters * 3);

  auto one_iter = [&](bool timed) {
    for (int k = 0; k < 3; ++k) {
      const std::uint32_t r = lcg(rng);
      const Side side = (r & 1u) ? Side::Buy : Side::Sell;
      const PriceTicks price = static_cast<PriceTicks>(10000 + (r % 20));
      const Qty qty = static_cast<Qty>(1 + (r % 5));
      const auto res = book.add_limit(id, qty, side, price);
      if (!timed) do_not_optimize(res.accepted);
      cancellable.push_back(id);
      ++id;
    }

    if (!cancellable.empty()) {
      const OrderId victim = cancellable.back();
      cancellable.pop_back();
      const bool ok = book.cancel(victim);
      if (!timed) do_not_optimize(ok);
    }

    const std::uint32_t r2 = lcg(rng);
    const Side aggressive_side = (r2 & 1u) ? Side::Buy : Side::Sell;
    const PriceTicks aggressive_price = aggressive_side == Side::Buy ? 20000 : 1;
    const auto res2 = book.add_limit(id++, 1, aggressive_side, aggressive_price);
    if (!timed) do_not_optimize(res2.accepted);
  };

  for (std::size_t i = 0; i < warmup_iters; ++i) one_iter(false);

  const std::uint64_t new_before = g_new_calls.load(std::memory_order_relaxed);

  const std::uint64_t t0 = ns_now();
  for (std::size_t i = 0; i < iters; ++i) one_iter(true);
  const std::uint64_t t1 = ns_now();

  const std::uint64_t new_after = g_new_calls.load(std::memory_order_relaxed);

  report("mixed_stream", iters * 5, (t1 - t0));
  check_allocs("mixed_stream", new_before, new_after);
}

int main() {
  constexpr std::size_t MAX_ORDERS = 5'000'000;
  constexpr std::size_t OPS = 2'000'000;
  constexpr std::size_t WARMUP = 200'000;

  bench_add_resting(MAX_ORDERS, WARMUP, OPS, 1);
  bench_cancel(MAX_ORDERS, WARMUP / 10, OPS / 2, 1);
  bench_marketable_match(MAX_ORDERS, WARMUP, OPS, 1);
  bench_mixed_stream(MAX_ORDERS, 50'000, 500'000, 1);

  std::cout << "process_total_new_calls="
            << g_new_calls.load(std::memory_order_relaxed)
            << "\n";
  return 0;
}

