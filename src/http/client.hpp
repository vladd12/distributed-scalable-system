#pragma once

#include <http/request.hpp>
#include <http/response.hpp>
//
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
//
#include <functional>
#include <memory>

namespace http
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/// \brief Type alias for the client request completion callback.
using response_callback = std::function<void(boost::system::error_code, const response &)>;

/// \brief Manages a single HTTP client connection: resolves, connects, sends request,
///        reads response and invokes callback.
class client_session : public std::enable_shared_from_this<client_session>
{
public:
  using pointer = std::shared_ptr<client_session>;

  /// \brief Creates a new client session.
  /// \param io_context The io_context for asynchronous operations.
  /// \param host Remote host (domain or IP).
  /// \param port Remote port (e.g., "80").
  static pointer create(asio::io_context &io_context);

  /// \brief Starts the asynchronous operation: resolve and connect to the host on specified port.
  void async_connect(const std::string &host, const std::uint16_t port);

  /// \brief   Starts the synchronous operation: resolve and connect to the host on specified port.
  /// \details This blocked operation for execution thread.
  void sync_connect(const std::string &host, const std::uint16_t port);

  void write(const request &req, response_callback handler);

private:
  client_session(asio::io_context &io_context);

  void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results);
  void do_connect(tcp::resolver::results_type results);
  void on_connect(boost::system::error_code ec);

  void on_write_request(boost::system::error_code ec, std::size_t bytes_transferred);

  void read_all();
  void on_request_parse(boost::system::error_code ec, std::size_t bytes_transferred);

  void finish(boost::system::error_code ec);

  tcp::socket m_socket;
  tcp::resolver m_resolver;

  response_callback m_handler;

  asio::streambuf m_buffer;
  response m_response;
  std::string m_request;
  std::size_t m_content_length = 0;
  bool m_keep_alive = false;
};

/// \brief Minimalistic asynchronous HTTP/1.1 client built on boost::asio.
class http_client : public boost::noncopyable
{
public:
  /// \brief Constructs the client.
  /// \param ctx The io_context that drives async operations.
  explicit http_client(asio::io_context &ctx, const std::string &host, const std::uint16_t port);

  /// \brief Performs an asynchronous HTTP GET request.
  void get(const std::string &path, const headers_t &headers, response_callback handler);

  /// \brief Performs an asynchronous HTTP POST request with a body.
  void post(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler);

  /// \brief Performs an asynchronous HTTP PUT request with a body.
  void put(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler);

  /// \brief Performs an asynchronous HTTP DELETE request.
  void delete_(const std::string &path, const headers_t &headers, response_callback handler);

private:
  /// \brief Performs a generic asynchronous HTTP request.
  void async_request(request req, response_callback handler);

  std::string m_host;
  client_session::pointer m_session;
};

} // namespace http
