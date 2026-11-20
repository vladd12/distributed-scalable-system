#include <boost/asio/io_context.hpp>
//
#include <core/errors.hpp>
#include <gtest/gtest.h>
#include <http/backoff.hpp>
//
#include <chrono>
#include <cstdint>
#include <functional>

using namespace std::chrono_literals;

namespace details = http::details;
namespace asio = boost::asio;

// ============================================================================
// Strategies
// ============================================================================

TEST(backoff_strategy_test, const_strategy_never_changes)
{
  details::const_strategy<std::chrono::microseconds> s(100us);
  EXPECT_EQ(s.get_current(), 100us);
  s.next();
  EXPECT_EQ(s.get_current(), 100us);
  s.next();
  EXPECT_EQ(s.get_current(), 100us);
}

TEST(backoff_strategy_test, const_strategy_reset)
{
  details::const_strategy<std::chrono::microseconds> s(100us);
  s.reset(500us);
  EXPECT_EQ(s.get_current(), 500us);
}

TEST(backoff_strategy_test, scale_strategy_multiplies)
{
  details::scale_strategy<std::chrono::microseconds, 3> s(10us);
  EXPECT_EQ(s.get_current(), 10us);
  s.next();
  EXPECT_EQ(s.get_current(), 30us);
  s.next();
  EXPECT_EQ(s.get_current(), 90us);
  s.next();
  EXPECT_EQ(s.get_current(), 270us);
}

TEST(backoff_strategy_test, scale_strategy_reset)
{
  details::scale_strategy<std::chrono::microseconds, 3> s(10us);
  s.next();
  s.next();
  EXPECT_EQ(s.get_current(), 90us);
  s.reset(10us);
  EXPECT_EQ(s.get_current(), 10us);
}

TEST(backoff_strategy_test, scale_strategy_overflow_protection)
{
  // Small rep type to trigger the limit quickly.
  using small_time = std::chrono::duration<std::int32_t, std::micro>;
  details::scale_strategy<small_time, 10> s(small_time(1000));

  for (int i = 0; i < 20; ++i)
    s.next();

  // Must stay within representable range (no UB, no wrap).
  EXPECT_LE(s.get_current().count(), small_time::max().count());
  EXPECT_GT(s.get_current().count(), 0);
}

TEST(backoff_strategy_test, exponential_is_scale_by_2)
{
  details::exponential_strategy<std::chrono::microseconds> s(10us);
  s.next();
  EXPECT_EQ(s.get_current(), 20us);
  s.next();
  EXPECT_EQ(s.get_current(), 40us);
  s.next();
  EXPECT_EQ(s.get_current(), 80us);
}

// ============================================================================
// Waiter
// ============================================================================

TEST(backoff_waiter_test, wait_fires_after_initial_delay)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  bool fired = false;
  auto start = std::chrono::steady_clock::now();
  waiter->wait([&](boost::system::error_code ec, bool is_last) {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_FALSE(ec);
    EXPECT_FALSE(is_last);
    EXPECT_GE(elapsed, details::default_initial_wait_time);
    fired = true;
  });
  ctx.run();

  EXPECT_TRUE(fired);
}

TEST(backoff_waiter_test, retries_until_exhausted)
{
  asio::io_context ctx;
  constexpr std::size_t max_retries = 3;
  auto waiter = std::make_shared<details::const_backoff_waiter<std::chrono::microseconds>>(ctx, 10us, max_retries);

  int retries = 0;
  bool gave_up = false;

  std::function<void(boost::system::error_code, bool)> handler;
  handler = [&](boost::system::error_code, bool is_last) {
    if (is_last)
    {
      gave_up = true;
      return;
    }
    ++retries;
    waiter->wait(handler);
  };

  waiter->wait(handler);
  ctx.run();

  EXPECT_EQ(retries, max_retries);
  EXPECT_TRUE(gave_up);
}

TEST(backoff_waiter_test, exhausted_immediately_synchronously)
{
  asio::io_context ctx;
  auto waiter = std::make_shared<details::const_backoff_waiter<std::chrono::microseconds>>(ctx, 10us, 0); // 0 retries

  bool fired_before_run = false;
  bool last_flag = false;

  waiter->wait([&](boost::system::error_code ec, bool is_last) {
    EXPECT_FALSE(ec);
    fired_before_run = true;
    last_flag = is_last;
  });

  // Give-up path is synchronous: handler must have fired before ctx.run().
  EXPECT_TRUE(fired_before_run);
  EXPECT_TRUE(last_flag);

  // Nothing left to run; this is just a sanity check that it returns immediately.
  ctx.run();
}

TEST(backoff_waiter_test, double_wait_throws)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  waiter->wait([](auto, auto) {});
  EXPECT_THROW(waiter->wait([](auto, auto) {}), core::http_error);

  // Clean up the pending async_wait to keep sanitizers / leak checkers happy.
  (void)waiter->cancel();
}

TEST(backoff_waiter_test, cancel_stops_pending_wait)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  bool fired = false;
  waiter->wait([&](auto, auto) { fired = true; });

  const std::size_t cancelled = waiter->cancel();
  EXPECT_EQ(cancelled, 1u); // one pending async_wait

  ctx.run();
  EXPECT_FALSE(fired); // handler must not be invoked after cancel
}

TEST(backoff_waiter_test, cancel_without_pending_returns_zero)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  EXPECT_EQ(waiter->cancel(), 0u); // nothing pending yet
}

TEST(backoff_waiter_test, reset_after_give_up_restores_state)
{
  asio::io_context ctx;
  constexpr std::size_t max_retries = 2;
  auto waiter = std::make_shared<details::const_backoff_waiter<std::chrono::microseconds>>(ctx, 10us, max_retries);

  auto run_cycle = [&]() -> std::pair<int, bool> {
    int retries = 0;
    bool gave_up = false;
    std::function<void(boost::system::error_code, bool)> handler;
    handler = [&](boost::system::error_code, bool is_last) {
      if (is_last)
      {
        gave_up = true;
        return;
      }
      ++retries;
      waiter->wait(handler);
    };
    waiter->wait(handler);
    ctx.run();
    ctx.restart(); // allow reuse of io_context after run() returned
    return { retries, gave_up };
  };

  auto [r1, g1] = run_cycle();
  EXPECT_EQ(r1, max_retries);
  EXPECT_TRUE(g1);

  (void)waiter->reset();

  auto [r2, g2] = run_cycle();
  EXPECT_EQ(r2, max_retries);
  EXPECT_TRUE(g2);
}

TEST(backoff_waiter_test, reset_during_pending_wait_cancels)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  bool fired = false;
  waiter->wait([&](auto, auto) { fired = true; });

  const std::size_t cancelled = waiter->reset();
  EXPECT_EQ(cancelled, 1u);

  ctx.run();
  EXPECT_FALSE(fired);
}

TEST(backoff_waiter_test, cancel_then_wait_works_again)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  waiter->wait([](auto, auto) {});
  (void)waiter->cancel();
  ctx.run();
  ctx.restart();

  bool fired = false;
  waiter->wait([&](boost::system::error_code ec, bool is_last) {
    EXPECT_FALSE(ec);
    EXPECT_FALSE(is_last);
    fired = true;
  });
  ctx.run();

  EXPECT_TRUE(fired);
}

TEST(backoff_waiter_test, empty_handler_throws)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);
  EXPECT_THROW(waiter->wait(nullptr), core::http_error);
  EXPECT_THROW(waiter->wait({}), core::http_error); // default-constructed std::function
}

TEST(backoff_waiter_test, give_up_does_not_wait_initial_delay)
{
  asio::io_context ctx;
  // Initial = 5 seconds. If give-up waited for it, this test would take 5s.
  auto waiter
      = std::make_shared<details::const_backoff_waiter<std::chrono::microseconds>>(ctx, std::chrono::seconds(5), 0);

  const auto start = std::chrono::steady_clock::now();
  bool fired = false;
  waiter->wait([&](auto, bool is_last) {
    fired = true;
    EXPECT_TRUE(is_last);
  });
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_TRUE(fired);
  EXPECT_LT(elapsed, std::chrono::milliseconds(100)); // way under 5s
}

TEST(backoff_waiter_test, cancel_after_handler_fired_returns_zero)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  waiter->wait([](auto, auto) {});
  ctx.run(); // handler already done

  EXPECT_EQ(waiter->cancel(), 0u); // nothing to cancel
}

TEST(backoff_waiter_test, double_cancel_is_idempotent)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  waiter->wait([](auto, auto) {});
  EXPECT_EQ(waiter->cancel(), 1u); // first time cancels pending
  EXPECT_EQ(waiter->cancel(), 0u); // second time - nothing
  ctx.run();
}

TEST(backoff_waiter_test, throw_on_double_wait_keeps_pending_alive)
{
  asio::io_context ctx;
  auto waiter = details::create_default_const_backoff_waiter(ctx);

  bool first_fired = false;
  waiter->wait([&](auto, auto) { first_fired = true; });

  EXPECT_THROW(waiter->wait([](auto, auto) {}), core::http_error);

  ctx.run();
  EXPECT_TRUE(first_fired); // first handler done, second was discarded
}

TEST(backoff_waiter_test, max_retries_1_yields_one_retry)
{
  asio::io_context ctx;
  auto waiter = std::make_shared<details::const_backoff_waiter<std::chrono::microseconds>>(ctx, 1us, 1);

  int retries = 0;
  bool gave_up = false;
  std::function<void(boost::system::error_code, bool)> h;
  h = [&](auto, bool is_last) {
    if (is_last)
    {
      gave_up = true;
      return;
    }
    ++retries;
    waiter->wait(h);
  };
  waiter->wait(h);
  ctx.run();

  EXPECT_EQ(retries, 1);
  EXPECT_TRUE(gave_up);
}

TEST(backoff_waiter_test, zero_initial_wait_time)
{
  asio::io_context ctx;
  auto waiter = std::make_shared<details::const_backoff_waiter<std::chrono::microseconds>>(ctx, 0us, 3);

  int retries = 0;
  bool gave_up = false;
  std::function<void(boost::system::error_code, bool)> h;
  h = [&](auto, bool is_last) {
    if (is_last)
    {
      gave_up = true;
      return;
    }
    ++retries;
    waiter->wait(h);
  };
  waiter->wait(h);
  ctx.run();

  EXPECT_EQ(retries, 3);
  EXPECT_TRUE(gave_up);
}

TEST(backoff_waiter_test, reset_after_zero_max_still_gives_up)
{
  asio::io_context ctx;
  auto waiter = std::make_shared<details::const_backoff_waiter<std::chrono::microseconds>>(ctx, 10us, 0);

  int fires = 0;
  for (int i = 0; i < 3; ++i)
  {
    bool is_last = false;
    waiter->wait([&](auto, bool last) {
      ++fires;
      is_last = last;
    });
    EXPECT_TRUE(is_last); // every time give-up, because max_retries = 0
    (void)waiter->reset();
  }
  EXPECT_EQ(fires, 3);
}
