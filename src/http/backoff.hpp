#pragma once

#include <boost/asio/steady_timer.hpp>
#include <core/errors.hpp>
#include <functional>
#include <memory>

namespace http::details
{
namespace asio = boost::asio;

/// @brief Default multiplicative factor used by @ref scale_strategy.
static constexpr inline std::size_t default_scale = 5ull;
/// @brief Default number of retries performed by @ref base_backoff_waiter.
static constexpr inline std::size_t default_max_retries = 5ull;
/// @brief Default initial delay before the first retry.
static constexpr inline asio::chrono::microseconds default_initial_wait_time = asio::chrono::microseconds(10);

// ============================================================================
// Strategies
// ============================================================================

/// @brief Backoff strategy that keeps the wait time constant.
/// @details Every call to next() leaves the current value untouched. Useful
///          when a fixed retry interval is desired (e.g. polling a resource).
/// @tparam  Time Duration type used for the wait interval
///          (defaults to std::chrono::microseconds).
template <typename Time = asio::chrono::microseconds> //
class const_strategy final
{
private:
  Time m_current; ///< Currently configured wait interval.

public:
  /// @brief Constructs the strategy with a fixed wait interval.
  /// @param current Initial and constant wait interval.
  inline const_strategy(Time current) noexcept : m_current(current)
  {
  }

  /// @brief Returns the current wait interval.
  [[nodiscard]] inline Time get_current() const noexcept
  {
    return m_current;
  }

  /// @brief No-op: the interval never changes.
  inline void next() noexcept
  {
  }

  /// @brief Overrides the current wait interval with @p value.
  /// @param value New wait interval to be returned by get_current().
  inline void reset(Time value) noexcept
  {
    m_current = value;
  }
};

/// @brief Backoff strategy that multiplies the interval by a fixed factor.
/// @details After each next() the current interval is multiplied by @p Scale.
///          The multiplication is guarded against overflow: once the interval
///          approaches `Time::max() / Scale`, further next() calls become no-ops.
/// @tparam  Time  Duration type used for the wait interval.
/// @tparam  Scale Multiplicative factor applied by next() (defaults to
///          @ref default_scale).
template <typename Time = asio::chrono::microseconds, std::size_t Scale = default_scale> //
class scale_strategy final
{
private:
  static constexpr inline std::size_t scale = Scale; ///< Multiplicative factor.
  Time m_current;                                    ///< Currently configured wait interval.

public:
  /// @brief Constructs the strategy with a starting wait interval.
  /// @param current Initial wait interval before any next() call.
  inline scale_strategy(Time current) noexcept : m_current(current)
  {
  }

  /// @brief Returns the current wait interval.
  [[nodiscard]] inline Time get_current() const noexcept
  {
    return m_current;
  }

  /// @brief   Advances the interval by multiplying it by @ref scale.
  /// @details If the current value would overflow when multiplied, the
  ///          interval is left unchanged (no UB, no wrap-around).
  inline void next() noexcept
  {
    constexpr auto limit = Time::max() / scale;
    if (m_current < limit)
      m_current = Time { static_cast<Time::rep>(m_current.count() * scale) };
  }

  /// @brief Resets the current wait interval to @p value.
  /// @param value New wait interval to be returned by get_current().
  inline void reset(Time value) noexcept
  {
    m_current = value;
  }
};

/// @brief   Backoff strategy with a factor of 2 (classic exponential backoff).
/// @details Alias for @ref scale_strategy with `Scale == 2`. Named separately
///          because "exponential backoff" is a well-established term and reads
///          better at the call site than `scale_strategy<Time, 2>`.
template <typename Time> //
using exponential_strategy = scale_strategy<Time, 2ull>;

// ============================================================================
// Handler
// ============================================================================

/// @brief Callback invoked by @ref base_backoff_waiter after each wait.
/// @param ec          Error code from the timer (empty on normal expiry,
///                    `asio::error::operation_aborted` on cancellation,
///                    though cancelled handlers are silently dropped and
///                    never reach the user callback).
/// @param is_last_try `true` if no further retries will be scheduled — the
///                    caller should treat this as terminal failure and stop
///                    retrying. `false` on every intermediate retry.
using backoff_handler = std::function<void(const boost::system::error_code /*ec*/, const bool /*is_last_try*/)>;

// ============================================================================
// Waiter
// ============================================================================

/// @brief Asynchronous retry driver that applies a strategy between attempts.
///
/// @details Typical use:
///          - call wait() with a handler;
///          - after the timer fires, the handler is invoked with `is_last_try == false`;
///          - the handler decides whether to retry (call wait() again) or give up;
///          - once `m_current_retry > m_max_retries`, the next wait() invokes the
///            handler synchronously with `is_last_try == true`.
///
/// @note Not thread-safe. All public methods must be invoked from the same
///       thread / strand as the associated io_context.
/// @note The handler is invoked at most once per wait() call. It is NOT invoked
///       if cancel() or reset() is called while a wait is pending, or if the
///       waiter (and its io_context) is destroyed before the timer fires.
/// @note A handler that throws propagates the exception out of the current
///       io_context::run() invocation (in the async path) or out of wait()
///       (in the synchronous give-up path). Internal state is updated before
///       the call, so the waiter remains usable afterwards.
///
/// @tparam Time     Duration type used for wait intervals.
/// @tparam Strategy Strategy type that decides how the interval evolves
///                  between retries (defaults to @ref const_strategy).
template <typename Time = asio::chrono::microseconds, typename Strategy = const_strategy<Time>> //
class base_backoff_waiter final : public std::enable_shared_from_this<base_backoff_waiter<Time, Strategy>>
{
private:
  using self_base_t = std::enable_shared_from_this<base_backoff_waiter<Time, Strategy>>;
  using self_base_t::shared_from_this; ///< Pull shared_from_this into scope (dependent base).

  // clang-format off
  const Time m_initial_wait_time;  ///< Value used by reset() to restore the strategy.
  const std::size_t m_max_retries; ///< Maximum number of intermediate retries.

  Strategy m_strategy;             ///< Current strategy driving the wait interval.
  std::size_t m_current_retry;     ///< Number of the upcoming retry attempt (1-based).
  asio::steady_timer m_timer;      ///< Timer implementing the asynchronous wait.
  backoff_handler m_handler;       ///< Pending handler for the current wait() call, if any.
  // clang-format on

  /// @brief   Moves the pending handler out and invokes it (if non-empty).
  /// @details Used by both the async path (in on_wait_event) and the synchronous
  ///          give-up path (in wait). Clearing m_handler before invocation allows
  ///          the handler to call wait() re-entrantly on the same waiter.
  /// @param   ec          Error code forwarded to the handler.
  /// @param   is_last_try Terminal-failure flag forwarded to the handler.
  inline void cleanup_handler_call(const boost::system::error_code ec, const bool is_last_try)
  {
    auto handler = std::move(m_handler);
    m_handler = nullptr;
    if (handler)
      handler(ec, is_last_try);
  }

  /// @brief   Timer completion handler: advances state and invokes the user callback.
  /// @details Silently ignores cancellation (`operation_aborted`) and stale fires
  ///          when the handler has already been cleared by cancel()/reset().
  /// @param   ec Result of the timer wait.
  inline void on_wait_event(const boost::system::error_code ec)
  {
    // timer canceled or destructed - silence for us
    if (ec == asio::error::operation_aborted || !m_handler)
      return;

    ++m_current_retry; // SIZE_MAX -> 0 (wrap for unsigned, defined behavior)
    m_strategy.next();
    cleanup_handler_call(ec, false);
  }

public:
  /// @brief Constructs a waiter.
  /// @param ctx               io_context used to drive the internal timer.
  /// @param initial_wait_time Initial wait interval (also restored by reset()).
  /// @param max_retries       Maximum number of intermediate retries. A value of
  ///                          zero means that the first wait() will fire the
  ///                          handler synchronously with `is_last_try == true`.
  inline base_backoff_waiter(
      asio::io_context &ctx, const Time initial_wait_time, const std::size_t max_retries) noexcept
      : m_initial_wait_time(initial_wait_time)
      , m_max_retries(max_retries)
      , m_strategy(m_initial_wait_time)
      , m_current_retry(1ull)
      , m_timer(ctx)
  {
  }

  /// @brief   Schedules the next retry (or fires the handler synchronously if exhausted).
  /// @details If retries remain, arms the timer for `strategy.get_current()` and
  ///          stores @p handler to be invoked on expiry. If no retries remain,
  ///          invokes @p handler **synchronously** with `is_last_try == true`
  ///          without waiting — the caller can treat this as terminal failure.
  /// @param   handler Callback invoked after the wait (or synchronously on give-up).
  /// @throws  core::http_error if a wait is already pending (double-wait) or if
  ///          @p handler is empty.
  inline void wait(backoff_handler handler)
  {
    if (m_handler)
      throw core::http_error("backoff waiter already in wait-state with registered handler");
    if (!handler)
      throw core::http_error("backoff waiter: empty handler passed");

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

  /// @brief   Cancels a pending wait and clears the stored handler.
  /// @details Idempotent and safe to call when no wait is pending. The user
  ///          handler is never invoked as a result of cancellation.
  /// @return  Number of cancelled timer operations (0 or 1 in practice).
  [[nodiscard]] inline std::size_t cancel() noexcept
  {
    m_handler = nullptr;
    return m_timer.cancel();
  }

  /// @brief   Cancels any pending wait and restores the initial backoff state.
  /// @details Clears the retry counter and resets the strategy back to
  ///          `m_initial_wait_time`. Does NOT change @ref m_max_retries —
  ///          a waiter constructed with `max_retries == 0` still gives up
  ///          synchronously after reset().
  /// @return  Number of cancelled timer operations (0 or 1 in practice).
  [[nodiscard]] inline std::size_t reset() noexcept
  {
    m_current_retry = 1ull;
    m_strategy.reset(m_initial_wait_time);
    return cancel();
  }
};

/// @brief Waiter with a constant retry interval.
template <typename Time> //
using const_backoff_waiter = base_backoff_waiter<Time, const_strategy<Time>>;

/// @brief  Waiter whose interval grows by a fixed factor between retries.
/// @tparam Time  Duration type for wait intervals.
/// @tparam Scale Multiplicative factor applied after each retry.
template <typename Time, std::size_t Scale> //
using scale_backoff_waiter = base_backoff_waiter<Time, scale_strategy<Time, Scale>>;

/// @brief Waiter with classic exponential backoff (factor 2).
template <typename Time> //
using exponential_backoff_waiter = base_backoff_waiter<Time, exponential_strategy<Time>>;

// ============================================================================
// Factories
// ============================================================================

/// @brief  Creates a waiter with the default constant strategy.
/// @param  ctx io_context used to drive the internal timer.
/// @return Shared pointer to a const_backoff_waiter using @ref default_initial_wait_time
///         and @ref default_max_retries.
[[nodiscard]] inline auto create_default_const_backoff_waiter(asio::io_context &ctx)
{
  return std::make_shared<const_backoff_waiter<asio::chrono::microseconds>>(
      ctx, default_initial_wait_time, default_max_retries);
}

/// @brief  Creates a waiter with the default scale strategy.
/// @param  ctx io_context used to drive the internal timer.
/// @return Shared pointer to a scale_backoff_waiter with @ref default_scale,
///         @ref default_initial_wait_time and @ref default_max_retries.
[[nodiscard]] inline auto create_default_scale_backoff_waiter(asio::io_context &ctx)
{
  return std::make_shared<scale_backoff_waiter<asio::chrono::microseconds, default_scale>>(
      ctx, default_initial_wait_time, default_max_retries);
}

/// @brief  Creates a waiter with the default exponential (factor 2) strategy.
/// @param  ctx io_context used to drive the internal timer.
/// @return Shared pointer to an exponential_backoff_waiter with
///         @ref default_initial_wait_time and @ref default_max_retries.
[[nodiscard]] inline auto create_default_exponential_backoff_waiter(asio::io_context &ctx)
{
  return std::make_shared<exponential_backoff_waiter<asio::chrono::microseconds>>(
      ctx, default_initial_wait_time, default_max_retries);
}

} // namespace http::details
