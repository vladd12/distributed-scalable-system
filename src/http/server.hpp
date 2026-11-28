#pragma once

#include <http/backoff.hpp>
#include <http/request.hpp>
#include <http/response.hpp>
//
#include <boost/asio.hpp>
//
#include <functional>
#include <unordered_set>

namespace http
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/// @brief   Synchronous request handler.
/// @details Invoked on the io_context thread. If it throws, the session turns
///          the exception into a 500 response and closes the connection.
///          Should not block the thread - long work must be offloaded elsewhere.
using request_handler = std::function<response(const request &)>;

/// @brief   Minimalistic HTTP/1.1 server built on boost::asio.
/// @details Accepts connections in a loop; one server_session per connection.
///          Routing is by exact (method, path) match — no prefix or pattern matching.
///          Not thread-safe: all methods should be called from the io_context thread.
class server final
{
public:
  /// @brief  Constructs the server and begins accepting connections.
  /// @param  ctx   The io_context that drives async operations.
  /// @param  port  The TCP port to listen on (all interfaces, 0.0.0.0).
  /// @throws boost::system::system_error if the port cannot be bound (e.g. busy).
  explicit server(asio::io_context &ctx, const std::uint16_t port, http_configuration cfg = {});

  ~server() noexcept;

  // To avoid copy client objects.
  server(const server &) = delete;
  server &operator=(const server &) = delete;

  /// @brief   Registers a handler for a specific HTTP method and path.
  /// @details Matching is exact: `"/foo"` does not match `"/foo/bar"` or `"/foo?x=1"`.
  ///          Registering the same (method, path) twice silently overwrites the previous handler.
  void route(method m, const std::string &path, request_handler handler);

  /// @brief   Sets a fallback handler for requests that match no route.
  /// @details Default (until overridden) responds 404 Not Found.
  void set_default_handler(request_handler handler);

  /// @brief   Gracefully stops the server.
  /// @details Stops accepting new connections (closes the acceptor) and
  ///          cancels all active sessions (closes their sockets). Idempotent.
  /// @note    The actual socket cleanup happens on the io_context thread:
  ///          this method schedules the work via `asio::post` and returns
  ///          immediately. The caller must continue running `io_context::run()`
  ///          after `stop()` returns, until `active_sessions() == 0` AND no
  ///          further handlers are pending; only then is it safe to destroy
  ///          the io_context and the server object.
  /// @note    The server object must outlive all its sessions. Destroying the
  ///          server while a session is still alive results in dangling
  ///          `this` in the session's shutdown callback.
  void stop();

  /// @brief Returns the number of currently active sessions.
  [[nodiscard]] std::size_t active_sessions() const;

  /// @brief Returns true if stop() has been called, either explicitly by the
  ///        user or internally (fatal accept error / exhausted accept retries).
  [[nodiscard]] bool is_stopped() const;

private:
  class server_session;
  using session_ptr = std::shared_ptr<server_session>;

  /// @brief   Accepts the next incoming connection and starts a server_session for it.
  /// @details Asynchronous accept loop: on success, creates a server_session with
  ///          the dispatching handler (see dispatch()) and starts reading from it,
  ///          then re-arms the acceptor for the next connection.
  ///          The session object is kept alive by the async read chain
  ///          (shared_from_this in session::do_read).
  void do_accept();

  /// @brief   Routes a parsed request to the matching handler.
  /// @param   req Parsed request; lookup key is (req.line.method_type, req.line.path).
  /// @return  Response produced by the matched handler or by the default handler.
  /// @details Looks up the exact (method, path) key in `m_routes`. If no route
  ///          matches, delegates to `m_default_handler` (404 Not Found by default).
  ///          Matching is exact: no prefix matching, no path normalization,
  ///          query string is part of the key. See route_key for details.
  ///          Does not catch exceptions thrown by handlers - those are turned
  ///          into a 500 response upstream, in server_session::on_message_parsed.
  [[nodiscard]] response dispatch(const request &req);

  /// @brief  Registers a session for lifecycle tracking.
  /// @return true if the session was registered; false if the server is
  ///         stopping (caller must discard the session).
  bool track_session(const session_ptr &session);

  /// @brief   Removes a session from the active set, called from the session's shutdown path.
  /// @details Invoked exactly once per tracked session (via the base
  ///          `session::shutdown()` hook). Erasing an already-removed session is a no-op.
  void untrack_session(const session_ptr &session);

  /// @brief   Key for exact-match routing: (method, path).
  /// @details `path` is compared verbatim (no normalization — send "/Health" and
  ///          "/health" are different keys). Query string is part of the path.
  struct route_key
  {
    method method_type; ///< HTTP method.
    std::string path;   ///< Request target as received (including query, if any).
    bool operator==(const route_key &other) const = default;
  };

  /// @brief Hash combiner for route_key.
  struct route_key_hash
  {
    std::size_t operator()(const route_key &key) const noexcept;
  };

  asio::io_context &m_ctx;                                                 ///< Reference to the owning io_context.
  tcp::acceptor m_acceptor;                                                ///< Listening socket (0.0.0.0).
  std::unordered_map<route_key, request_handler, route_key_hash> m_routes; ///< Routes by (method, path).
  request_handler m_default_handler;                                       ///< Fallback if no route matches.
  http_configuration m_cfg;                                                ///< HTTP configuration for server sessions.
  mutable std::mutex m_sessions_mutex;                                     ///< Guards m_active_sessions and m_stopped.
  std::unordered_set<session_ptr> m_active_sessions;                       ///< Live sessions.
  bool m_stopped;                                                          ///< True after stop().
  std::shared_ptr<details::exponential_backoff_waiter<asio::chrono::microseconds>> m_waiter; ///< Backoff async waiter.
};

} // namespace http

using http_server_t = ::http::server;
