#pragma once

#include <http/request.hpp>
#include <http/response.hpp>
//
#include <boost/asio.hpp>
//
#include <functional>

namespace http
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/// @brief   Synchronous request handler.
/// @details Invoked on the io_context thread. If it throws, the session turns
///          the exception into a 500 response and closes the connection.
///          Should not block the thread — long work must be offloaded elsewhere.
using request_handler = std::function<response(const request &)>;

/// @brief   Minimalistic HTTP/1.1 server built on boost::asio.
/// @details Accepts connections in a loop; one server_session per connection.
///          Routing is by exact (method, path) match — no prefix or pattern matching.
///          Not thread-safe: all methods should be called from the io_context thread.
class server final
{
public:
  /// @brief  Constructs the server and begins accepting connections.
  /// @param  ctx  The io_context that drives async operations.
  /// @param  port  The TCP port to listen on (all interfaces, 0.0.0.0).
  /// @throws boost::system::system_error if the port cannot be bound (e.g. busy).
  explicit server(asio::io_context &ctx, const std::uint16_t port);

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

private:
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
  ///          Does not catch exceptions thrown by handlers — those are turned
  ///          into a 500 response upstream, in server_session::on_message_parsed.
  [[nodiscard]] response dispatch(const request &req);

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

  tcp::acceptor m_acceptor;                                                ///< Listening socket (0.0.0.0).
  std::unordered_map<route_key, request_handler, route_key_hash> m_routes; ///< Routes by (method, path).
  request_handler m_default_handler;                                       ///< Fallback if no route matches.
};

} // namespace http

using http_server_t = ::http::server;
