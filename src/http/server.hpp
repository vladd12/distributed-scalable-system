#pragma once

#include <http/request.hpp>
#include <http/response.hpp>
//
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
//
#include <chrono>
#include <functional>
#include <memory>

namespace http
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/// \brief Type alias for a request handler function.
using request_handler = std::function<response(const request &)>;

/// \brief Manages a single HTTP connection: reads a request, invokes the handler, and writes the response.
class http_session : public std::enable_shared_from_this<http_session>
{
public:
  using pointer = std::shared_ptr<http_session>;

  /// \brief Constructor
  http_session(tcp::socket socket, request_handler handler);

  /// \brief Creates a new session from an accepted socket.
  [[nodiscard]] static pointer create(tcp::socket socket, request_handler handler);

  /// \brief Starts reading the request from the socket.
  void start();

private:
  static constexpr std::size_t max_header_size = 16 * 1024;
  static constexpr std::size_t max_body_size = 1024 * 1024;
  static constexpr std::chrono::seconds io_timeout { 10 };

  /// \brief Async reading incoming HTTP request.
  void do_read();

  /// \brief Async reading the rest pasrt of the HTTP request.
  void do_remaining_read(const std::size_t remaining);

  /// \brief Parsing the incoming HTTP request from buffer with headers and possible part of body.
  void on_request_parse(boost::system::error_code ec, std::size_t bytes_transferred);

  /// \brief Adding remaining part of the HTTP request's body to the HTTP request structure.
  void on_remaining_data_read(boost::system::error_code ec, std::size_t bytes_transferred);

  void process_request();
  void do_write();
  void start_timeout();
  void cancel_timeout();
  void shutdown();

  tcp::socket m_socket;
  request_handler m_handler;
  asio::steady_timer m_timer;
  asio::streambuf m_buffer;
  std::size_t m_expected_body_size;
  request m_request;
  std::string m_response_data;
};

/// \brief Minimalistic asynchronous HTTP/1.1 server built on boost::asio.
class http_server : public boost::noncopyable
{
public:
  /// \brief Constructs the server and begins accepting connections.
  /// \param ctx  The io_context that drives async operations.
  /// \param port  The TCP port to listen on.
  explicit http_server(asio::io_context &ctx, const std::uint16_t port);

  /// \brief Registers a handler for a specific HTTP method and path.
  void route(method m, const std::string &path, request_handler handler);

  /// \brief Sets a fallback handler for requests that match no route.
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
