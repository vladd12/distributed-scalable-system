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

/// @brief Type alias for the client request completion callback.
using response_callback = std::function<void(boost::system::error_code, const response &)>;

/// @brief Type alias for the client async connection callback.
using on_connect_callback = std::function<void(boost::system::error_code)>;

/// @brief Minimalistic asynchronous HTTP/1.1 client built on boost::asio.
class client final : public std::enable_shared_from_this<client>
{
private:
  class client_session;
  using client_session_ptr = std::shared_ptr<client_session>;

  std::string m_host;
  client_session_ptr m_session;

public:
  /// @brief Representation of the client connection type.
  enum class connect_type
  {
    sync = 0,
    async
  };

  /// @brief Constructs the client.
  /// @param ctx The io_context that drives async operations.
  explicit client(asio::io_context &ctx, const std::string &host, const std::uint16_t port,
      const connect_type conn_type = connect_type::sync);

  // To avoid copy client objects.
  client(const client &) = delete;
  client &operator=(const client &) = delete;

  /// @brief Async execution for client that selected to be async on connection to the target
  void on_connected(on_connect_callback handler);

  /// @brief Performs an asynchronous HTTP GET request.
  void get(const std::string &path, const headers_t &headers, response_callback handler);

  /// @brief Performs an asynchronous HTTP POST request with a body.
  void post(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler);

  /// @brief Performs an asynchronous HTTP PUT request with a body.
  void put(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler);

  /// @brief Performs an asynchronous HTTP DELETE request.
  void delete_(const std::string &path, const headers_t &headers, response_callback handler);

private:
  /// @brief Performs a generic asynchronous HTTP request.
  void async_request(request req, response_callback handler);
};

} // namespace http

using http_client_t = ::http::client;
