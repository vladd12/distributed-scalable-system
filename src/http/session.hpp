#pragma once

#include <core/errors.hpp>
#include <http/common.hpp>
//
#include <boost/asio.hpp>
//
#include <istream>
#include <memory>
#include <string>

namespace http
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/// @brief   Common HTTP session configuration.
/// @details Used by both client and server sessions.
struct http_configuration
{
  std::chrono::milliseconds io_timeout { 10'000 }; ///< Per-operation timeout
  std::size_t max_header_size = 16 * 1024;         ///< Max bytes before CRLF CRLF
  std::size_t max_body_size = 1024 * 1024;         ///< Max body bytes (Content-Length)
};

/// @brief   Shared HTTP/1.1 state machine, parameterized by role.
/// @tparam  Derived  CRTP-derived session (client_session or server_session).
/// @tparam  R        Fully-parsed read message type (request for server, response for client).
/// @tparam  W        Write message type (response for server, request for client).
/// @details Base handles: read headers, parse, size checks, read body, timeout,
///          write, shutdown. Derived only decides *what* to do at the three hooks.
///
/// Derived must provide:
///   - using read_t  = R;
///   - using write_t = W;
///   - void on_message_parsed(read_t msg);           // called on fully-read message
///   - void on_write_complete();                     // called after successful write
///   - void on_error(boost::system::error_code ec);  // called on any failure
template <typename Derived, typename R, typename W> //
class session : public std::enable_shared_from_this<Derived>
{
public:
  using read_t = R;  ///< Type of incoming message
  using write_t = W; ///< Type of outgoing message
  using pointer = std::shared_ptr<Derived>;

protected:
  tcp::socket m_socket;       ///< Underlying TCP socket
  asio::steady_timer m_timer; ///< Per-operation timeout timer
  asio::streambuf m_buffer;   ///< Read buffer (header + body capacity)
  http_configuration m_cfg;   ///< Session configuration
  bool m_keep_alive = false;  ///< Whether peer agreed to keep-alive

  /// @brief Initializes the session around an already-open (or about-to-open) socket.
  /// @param socket Connected/owned socket (moved in).
  /// @param cfg    Session configuration (moved in).
  session(tcp::socket socket, http_configuration cfg)
      : m_socket(std::move(socket))
      , m_timer(m_socket.get_executor())
      , m_buffer(cfg.max_header_size + cfg.max_body_size)
      , m_cfg(std::move(cfg))
  {
  }

  // To avoid copy session objects.
  session(const session &) = delete;
  session &operator=(const session &) = delete;

  /// @brief   Returns the CRTP-derived instance.
  /// @details Safe for all currently supported call paths (no multiple inheritance).
  [[nodiscard]] inline Derived &derived() noexcept
  {
    return static_cast<Derived &>(*this);
  }

public:
  /// @brief   Cancels pending operations and closes the socket.
  /// @details Idempotent: safe to call multiple times. Clears the timer first
  ///          so a pending timeout cannot fire after the socket is closed.
  void shutdown()
  {
    cancel_timer();
    if (!m_socket.is_open())
      return;
    boost::system::error_code ignored;
    m_socket.cancel(ignored);
    m_socket.shutdown(tcp::socket::shutdown_both, ignored);
    m_socket.close(ignored);
  }

  /// @brief Returns true if the underlying socket is still open.
  [[nodiscard]] inline bool is_open() const noexcept
  {
    return m_socket.is_open();
  }

protected:
  /// @brief   Starts a read cycle (headers -> body -> on_message_parsed).
  /// @details Resets the timeout and initiates async_read_until("\r\n\r\n").
  ///          Should be called from derived::start() (server) or
  ///          on_write_complete (client).
  void do_read()
  {
    start_timer();
    auto self = derived().shared_from_this();
    asio::async_read_until(m_socket, m_buffer, "\r\n\r\n",
        [this, self](boost::system::error_code ec, std::size_t) { on_headers_read(ec); });
  }

  /// @brief   Serializes and writes a message, then calls on_write_complete.
  /// @param   msg Message to serialize and write (request or response).
  /// @details Serialized payload is kept alive by a shared_ptr captured into the
  ///          handler so asio::buffer has stable memory for the whole write.
  void do_write(const write_t &msg)
  {
    auto payload = std::make_shared<const std::string>(msg.serialize());
    start_timer();
    auto self = derived().shared_from_this();
    asio::async_write(
        m_socket, asio::buffer(*payload), [this, self, payload](boost::system::error_code ec, std::size_t) {
          cancel_timer();
          if (ec)
            derived().on_error(ec);
          else
            derived().on_write_complete();
        });
  }

private:
  /// @brief   Called when the header block (up to and including CRLF CRLF) has been read.
  /// @param   ec boost::asio error from async_read_until.
  /// @details Validates header size, parses the message, validates Content-Length
  ///          against max_body_size, and either reads the remaining body or
  ///          immediately dispatches via on_message_parsed.
  void on_headers_read(boost::system::error_code ec)
  {
    cancel_timer();

    if (ec == asio::error::operation_aborted)
      return; // we cancelled (shutdown/timeout)

    if (ec == asio::error::not_found)
    {
      // TODO: log error
      // derived().on_error(core::http_error("HTTP headers exceed configured buffer size"));
      derived().on_error(asio::error::message_size);
      return;
    }
    if (ec)
    {
      derived().on_error(ec);
      return;
    }

    // Exact header size = position of the CRLF CRLF delimiter + 4.
    constexpr static std::string_view crlf("\r\n\r\n");
    auto data = m_buffer.data();
    auto begin = asio::buffers_begin(data);
    auto end = asio::buffers_end(data);
    auto delim = std::search(begin, end, crlf.begin(), crlf.end());
    // Sanity check; async_read_until should have guaranteed the delimiter.
    if (delim == end)
    {
      // TODO: log error
      // derived().on_error(core::http_error("HTTP message has no CRLF CRLF delimiter"));
      derived().on_error(asio::error::invalid_argument);
      return;
    }
    const std::size_t header_size = static_cast<std::size_t>(std::distance(begin, delim)) + 4;
    if (header_size > m_cfg.max_header_size)
    {
      // TODO: log error
      // derived().on_error(core::http_error("HTTP headers exceed max_header_size"));
      derived().on_error(asio::error::message_size);
      return;
    }

    try
    {
      std::istream stream(&m_buffer);
      read_t msg = read_t::parse(stream);
      m_buffer.consume(m_buffer.size()); // no-op after parse; kept for symmetry with the read path

      const std::size_t expected = msg.headers.content_length();
      if (expected > m_cfg.max_body_size)
      {
        // TODO: log error
        // derived().on_error(core::http_error("HTTP body exceeds max_body_size"));
        derived().on_error(asio::error::message_size);
        return;
      }
      // Body already longer than declared Content-Length (pipelining or a malicious peer) - refuse the message.
      if (msg.body.size() > expected)
      {
        // TODO: log error
        // derived().on_error(core::http_error("HTTP body exceeds Content-Length (pipelining?)"));
        derived().on_error(asio::error::invalid_argument);
        return;
      }

      const std::size_t remaining = expected - msg.body.size();
      if (remaining > 0)
        do_remaining_read(remaining, std::move(msg));
      else
        derived().on_message_parsed(std::move(msg));
    } catch (const std::exception &ex)
    {
      // Malformed HTTP (bad status line, bad request line, etc.).
      // TODO: log error
      // derived().on_error(core::http_error(ex.what()));
      derived().on_error(asio::error::invalid_argument);
    }
  }

  /// @brief Reads exactly `remaining` more body bytes into the buffer.
  /// @param remaining Bytes left to complete the body (per Content-Length).
  /// @param msg       Partially-parsed message; moved into the handler.
  void do_remaining_read(std::size_t remaining, read_t msg)
  {
    start_timer();
    auto self = derived().shared_from_this();
    asio::async_read(m_socket, m_buffer, asio::transfer_exactly(remaining),
        [this, self, msg = std::move(msg)](
            boost::system::error_code ec, std::size_t) mutable { on_body_read(ec, std::move(msg)); });
  }

  /// @brief Appends the just-read body fragment and dispatches the message.
  /// @param ec  boost::asio error from async_read.
  /// @param msg Message whose body is to be completed.
  void on_body_read(boost::system::error_code ec, read_t msg)
  {
    cancel_timer();
    if (!ec)
    {
      msg.body.append(asio::buffers_begin(m_buffer.data()), asio::buffers_end(m_buffer.data()));
      m_buffer.consume(m_buffer.size());
      derived().on_message_parsed(std::move(msg));
    }
    else
      derived().on_error(ec);
  }

  /// @brief   Arms the per-operation timeout for the currently pending async op.
  /// @details If it fires, the session is shut down and on_error(timed_out) is invoked.
  void start_timer()
  {
    m_timer.expires_after(m_cfg.io_timeout);
    auto self = derived().shared_from_this();
    m_timer.async_wait([this, self](boost::system::error_code ec) {
      // normal cancellation — the async op completed successfully
      if (ec == asio::error::operation_aborted)
        return;
      shutdown();
      derived().on_error(ec ? ec : asio::error::timed_out);
    });
  }

  /// @brief Disarms the timeout timer. Safe to call when no timer is armed.
  void cancel_timer()
  {
    boost::system::error_code ignored;
    m_timer.cancel(ignored);
  }
};

} // namespace http
