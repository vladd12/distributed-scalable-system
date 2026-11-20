#pragma once

#include <boost/asio/steady_timer.hpp>
#include <core/errors.hpp>
#include <functional>
#include <memory>

namespace http::details
{
namespace asio = boost::asio;

static constexpr inline std::size_t default_scale = 5ull;
static constexpr inline std::size_t default_max_retries = 5ull;
static constexpr inline asio::chrono::microseconds default_initial_wait_time = asio::chrono::microseconds(10);

template <typename Time = asio::chrono::microseconds> //
class const_strategy final
{
private:
  Time m_current;

public:
  inline const_strategy(Time current) noexcept : m_current(current)
  {
  }

  [[nodiscard]] inline Time get_current() const noexcept
  {
    return m_current;
  }

  inline void next() noexcept
  {
  }

  inline void reset(Time value) noexcept
  {
    m_current = value;
  }
};

template <typename Time = asio::chrono::microseconds, std::size_t Scale = default_scale> //
class scale_strategy final
{
private:
  static constexpr inline std::size_t scale = Scale;
  Time m_current;

public:
  inline scale_strategy(Time current) noexcept : m_current(current)
  {
  }

  [[nodiscard]] inline Time get_current() const noexcept
  {
    return m_current;
  }

  inline void next() noexcept
  {
    constexpr auto limit = Time::max() / scale;
    if (m_current < limit)
      m_current = Time { static_cast<Time::rep>(m_current.count() * scale) };
  }

  inline void reset(Time value) noexcept
  {
    m_current = value;
  }
};

template <typename Time> //
using exponential_strategy = scale_strategy<Time, 2ull>;

using backoff_handler = std::function<void(const boost::system::error_code /*ec*/, const bool /*is_last_try*/)>;

template <typename Time = asio::chrono::microseconds, typename Strategy = const_strategy<Time>> //
class base_backoff_waiter final : public std::enable_shared_from_this<base_backoff_waiter<Time, Strategy>>
{
private:
  using self_base_t = std::enable_shared_from_this<base_backoff_waiter<Time, Strategy>>;
  using self_base_t::shared_from_this;

  const Time m_initial_wait_time;
  Strategy m_strategy;
  std::size_t m_max_retries;
  std::size_t m_current_retry;
  asio::steady_timer m_timer;
  backoff_handler m_handler;

  inline void cleanup_handler_call(const boost::system::error_code ec, const bool is_last_try)
  {
    auto handler = std::move(m_handler);
    m_handler = nullptr;
    if (handler)
      handler(ec, is_last_try);
  }

  inline void on_wait_event(const boost::system::error_code ec)
  {
    // timer canceled or destructed - silence for us
    if (ec == asio::error::operation_aborted || !m_handler)
      return;

    ++m_current_retry;
    m_strategy.next();
    cleanup_handler_call(ec, false);
  }

public:
  inline base_backoff_waiter(
      asio::io_context &ctx, const Time initial_wait_time, const std::size_t max_retries) noexcept
      : m_initial_wait_time(initial_wait_time)
      , m_strategy(m_initial_wait_time)
      , m_max_retries(max_retries)
      , m_current_retry(1ull)
      , m_timer(ctx)
  {
  }

  inline void wait(backoff_handler handler)
  {
    if (m_handler)
      throw core::http_error("backoff waiter already in wait-state with registered handler");

    m_handler = std::move(handler);
    if (m_current_retry > m_max_retries)
    {
      cleanup_handler_call({}, true);
    }
    else
    {
      m_timer.expires_after(m_strategy.get_current());
      m_timer.async_wait([self = shared_from_this()](const boost::system::error_code ec) { self->on_wait_event(ec); });
    }
  }

  [[nodiscard]] inline std::size_t cancel() noexcept
  {
    m_handler = nullptr;
    return m_timer.cancel();
  }

  [[nodiscard]] inline std::size_t reset() noexcept
  {
    m_current_retry = 1ull;
    m_strategy.reset(m_initial_wait_time);
    return cancel();
  }
};

template <typename Time> //
using const_backoff_waiter = base_backoff_waiter<Time, const_strategy<Time>>;

template <typename Time, std::size_t Scale> //
using scale_backoff_waiter = base_backoff_waiter<Time, scale_strategy<Time, Scale>>;

template <typename Time> //
using exponential_backoff_waiter = base_backoff_waiter<Time, exponential_strategy<Time>>;

[[nodiscard]] inline auto create_default_const_backoff_waiter(asio::io_context &ctx)
{
  return std::make_shared<const_backoff_waiter<asio::chrono::microseconds>>(
      ctx, default_initial_wait_time, default_max_retries);
}

[[nodiscard]] inline auto create_default_scale_backoff_waiter(asio::io_context &ctx)
{
  return std::make_shared<scale_backoff_waiter<asio::chrono::microseconds, default_scale>>(
      ctx, default_initial_wait_time, default_max_retries);
}

[[nodiscard]] inline auto create_default_exponential_backoff_waiter(asio::io_context &ctx)
{
  return std::make_shared<exponential_backoff_waiter<asio::chrono::microseconds>>(
      ctx, default_initial_wait_time, default_max_retries);
}

} // namespace http::details
