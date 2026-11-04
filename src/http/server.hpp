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

/// @brief Type alias for a request handler function.
using request_handler = std::function<response(const request &)>;

/// @brief Minimalistic asynchronous HTTP/1.1 server built on boost::asio.
class server final
{
public:
  /// @brief Constructs the server and begins accepting connections.
  /// @param ctx  The io_context that drives async operations.
  /// @param port  The TCP port to listen on.
  explicit server(asio::io_context &ctx, const std::uint16_t port);

  // To avoid copy client objects.
  server(const server &) = delete;
  server &operator=(const server &) = delete;

  /// @brief Registers a handler for a specific HTTP method and path.
  void route(method m, const std::string &path, request_handler handler);

  /// @brief Sets a fallback handler for requests that match no route.
  void set_default_handler(request_handler handler);

private:
  void do_accept();
  response dispatch(const request &req);

  struct route_key
  {
    method method_type;
    std::string path;
    bool operator==(const route_key &other) const = default;
  };

  struct route_key_hash
  {
    std::size_t operator()(const route_key &key) const;
  };

  tcp::acceptor m_acceptor;
  std::unordered_map<route_key, request_handler, route_key_hash> m_routes;
  request_handler m_default_handler;
};

} // namespace http

using http_server_t = ::http::server;
