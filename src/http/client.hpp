#pragma once

#include <http/response.hpp>
//
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>

namespace http
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/// \brief Type alias for the client request completion callback.
using response_callback = std::function<void(boost::system::error_code, response)>;

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
  /// \param req The HTTP request to send.
  /// \param handler Callback to be invoked when response is received or on error.
  static pointer create(asio::io_context &io_context, const std::string &host, const std::string &port,
      const std::string &req, response_callback handler);

  /// \brief Starts the asynchronous operation: resolve, connect, send, receive.
  void start();

private:
  client_session(asio::io_context &io_context, const std::string &host, const std::string &port, const std::string &req,
      response_callback handler);

  void do_resolve();
  void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results);
  void do_connect(tcp::resolver::results_type results);
  void on_connect(boost::system::error_code ec);
  void do_write_request();
  void on_write_request(boost::system::error_code ec, std::size_t bytes_transferred);
  void do_read_headers();
  void on_headers_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void do_read_body();
  void on_body_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void finish(boost::system::error_code ec);

  tcp::socket m_socket;
  tcp::resolver m_resolver;
  std::string m_host;
  std::string m_port;
  std::string m_request_data;
  response_callback m_handler;

  asio::streambuf m_buffer;
  response m_response;
  std::size_t m_content_length = 0;
  bool m_keep_alive = false;
};

/// \brief Minimalistic asynchronous HTTP/1.1 client built on boost::asio.
class http_client : public boost::noncopyable
{
public:
  /// \brief Constructs the client.
  /// \param ctx The io_context that drives async operations.
  explicit http_client(asio::io_context &ctx);

  /// \brief Performs an asynchronous HTTP GET request.
  void async_get(const std::string &host, const std::string &port, const std::string &path,
      const std::unordered_map<std::string, std::string> &headers, response_callback handler);

  /// \brief Performs an asynchronous HTTP POST request with a body.
  void async_post(const std::string &host, const std::string &port, const std::string &path,
      const std::unordered_map<std::string, std::string> &headers, const std::string &body, response_callback handler);

  /// \brief Performs an asynchronous HTTP PUT request with a body.
  void async_put(const std::string &host, const std::string &port, const std::string &path,
      const std::unordered_map<std::string, std::string> &headers, const std::string &body, response_callback handler);

  /// \brief Performs an asynchronous HTTP DELETE request.
  void async_delete(const std::string &host, const std::string &port, const std::string &path,
      const std::unordered_map<std::string, std::string> &headers, response_callback handler);

  /// \brief Performs a generic asynchronous HTTP request.
  void async_request(method m, const std::string &host, const std::string &port, const std::string &path,
      const std::unordered_map<std::string, std::string> &headers, const std::string &body, response_callback handler);

private:
  std::string build_request(method m, const std::string &host, const std::string &path,
      const std::unordered_map<std::string, std::string> &headers, const std::string &body) const;

  asio::io_context &m_ctx;
};

} // namespace http
