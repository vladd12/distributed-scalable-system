#pragma once

#include <boost/asio/steady_timer.hpp>
#include <functional>

namespace http::details
{

namespace asio = boost::asio;

static constexpr inline std::size_t default_scale = 2ull;

template <typename Time = asio::chrono::microseconds> //
class const_strategy final
{
private:
  Time m_current;

public:
  inline const_strategy(Time current) noexcept : m_current(current)
  {
  }

  inline Time get_current() const noexcept
  {
    return m_current;
  }

  inline Time next() const noexcept
  {
    return m_current;
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

  inline Time get_current() const noexcept
  {
    return m_current;
  }

  inline Time next() noexcept
  {
    m_current = m_current * scale;
    return m_current;
  }
};

using backoff_handler = std::function<void(boost::system::error_code)>;

template <typename Time = asio::chrono::microseconds, typename Strategy = const_strategy<Time>> //
class base_backoff_waiter final
{
private:
  Strategy m_strategy;
  std::size_t m_max_retries;
  std::size_t m_current_retry;
  asio::steady_timer m_timer;
  backoff_handler m_handler;

  void on_wait_event(const boost::system::error_code ec)
  {
  }

public:
  inline base_backoff_waiter(
      asio::io_context &ctx, const Time initial_wait_time, const std::size_t max_retries) noexcept
      : m_strategy(initial_wait_time)
      , m_max_retries(max_retries)
      , m_current_retry(1ull)
      , m_timer(ctx, initial_wait_time)
  {
  }

  void wait(backoff_handler handler)
  {
    m_handler = std::move(handler);
  }
};

} // namespace http::details
