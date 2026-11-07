#pragma once

#include <http/request.hpp>
#include <http/response.hpp>
//
#include <boost/asio.hpp>
//
#include <functional>
#include <memory>

namespace http
{

namespace asio = boost::asio;

/// @brief   Callback invoked when a request completes.
/// @details `ec` is non-zero on failure (connection closed, timeout, parse error);
///          `response` is only meaningful when `ec == 0`. Invoked exactly once per request.
using response_callback = std::function<void(boost::system::error_code, const response &)>;

/// @brief Callback invoked when an async connect attempt completes.
/// @details `ec` is non-zero on DNS or TCP failure.
using on_connect_callback = std::function<void(boost::system::error_code)>;

/// @brief   Minimalistic HTTP/1.1 client built on boost::asio.
/// @details Owns a single client_session; one TCP connection; one request in
///          flight at a time (a second call overwrites the pending callback).
///          Not thread-safe: all methods must be called from the io_context thread.
///          In `connect_type::sync` the constructor blocks and may throw;
///          in `connect_type::async` the caller must call `on_connected()` and
///          then run the `io_context` to actually establish the connection.
class client final : public std::enable_shared_from_this<client>
{
private:
  class client_session;
  using client_session_ptr = std::shared_ptr<client_session>;

  std::string m_host;
  client_session_ptr m_session;

public:
  /// @brief Client connection strategy.
  enum class connect_type
  {
    sync = 0, ///< Constructor blocks; throws core::http_error on failure.
    async     ///< on_connected(handler) drives connect; requires running io_context.
  };

  /// @brief  Constructs the client.
  /// @param  ctx       io_context that drives async operations.
  /// @param  host      Remote host (kept for Host header; used for connect).
  /// @param  port      Remote port.
  /// @param  conn_type sync or async connect strategy.
  /// @throws core::http_error if conn_type == sync and connect fails.
  /// @throws std::bad_alloc / boost errors possibly propagated from asio on async.
  explicit client(asio::io_context &ctx, const std::string &host, const std::uint16_t port,
      const connect_type conn_type = connect_type::sync, http_configuration cfg = {});

  // To avoid copy client objects.
  client(const client &) = delete;
  client &operator=(const client &) = delete;

  /// @brief   Registers the callback to be invoked after the async connect completes.
  /// @details No-op for sync mode (already connected). Must be called before
  ///          running the io_context; the handler is invoked once.
  void on_connected(on_connect_callback handler);

  /// @brief Performs an asynchronous HTTP GET request.
  /// @param path    Request target (path + optional query).
  /// @param headers Extra headers (Host and Connection filled if absent).
  /// @param handler Completion callback, invoked exactly once.
  void get(const std::string &path, const headers_t &headers, response_callback handler);

  /// @brief   Performs an asynchronous HTTP POST request with a body.
  /// @details Content-Length is derived from `body` unless already set in `headers`.
  void post(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler);

  /// @brief Performs an asynchronous HTTP PUT request with a body.
  /// @copydetails post
  void put(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler);

  /// @brief Performs an asynchronous HTTP DELETE request.
  void delete_(const std::string &path, const headers_t &headers, response_callback handler);

private:
  /// @brief   Performs a generic asynchronous HTTP request.
  /// @details Uses the underlying session. If no session is connected, invokes
  ///          handler with asio::error::not_connected.
  void async_request(request req, response_callback handler);
};

} // namespace http

using http_client_t = ::http::client;
