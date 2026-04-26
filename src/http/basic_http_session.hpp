#pragma once

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

/// TODO: move to json configuration
struct http_configuration
{
  std::uint64_t io_timeout = 5000;           ///< Common request reading/sending timeout, miliseconds.
  std::uint64_t max_header_size = 16 * 1024; ///< Max number of headers bytes, that client/server
                                             ///< can receive in HTTP response/request.
  std::uint64_t max_body_size = 1024 * 1024; ///< Max number of body bytes, that client/server can
                                             ///< receive in HTTP response/request.
};

template <typename role_traits> //
class basic_http_session : public std::enable_shared_from_this<basic_http_session<role_traits>>,
                           public boost::noncopyable
{
private:
  using base_shared_t = std::enable_shared_from_this<basic_http_session<role_traits>>;
  using read_t = role_traits::read_t;
  using write_t = role_traits::write_t;

public:
  using pointer = std::shared_ptr<basic_http_session<role_traits>>;
  using error_handler = std::function<void(const boost::system::error_code &, pointer)>;

  /// \brief Constructor.
  inline explicit basic_http_session(asio::io_context &io_ctx, http_configuration cfg)
      : m_configration(std::move(cfg))
      , m_socket(io_ctx)
      , m_timer(m_socket.get_executor())
      , m_buffer(cfg.max_header_size + cfg.max_body_size)
  //, m_role(std::move(role_traits {}))
  {
  }

  /// \brief Creates a new HTTP basic session.
  /// \param io_ctx The io_context for asynchronous operations.
  [[nodiscard]] inline static pointer create(asio::io_context &io_ctx, http_configuration cfg = http_configuration {})
  {
    return std::make_shared<basic_http_session<role_traits>>(io_ctx, std::move(cfg));
  }

  inline void async_read()
  {
    do_read();
  }

  inline void shutdown()
  {
    cancel_timer();
    m_socket.cancel(m_error);
    m_socket.shutdown(tcp::socket::shutdown_both, m_error);
    m_socket.close(m_error);
  }

private:
  inline void start_timer()
  {
    m_timer.expires_after(std::chrono::milliseconds { m_configration.io_timeout });
    m_timer.async_wait([self = base_shared_t::shared_from_this()](boost::system::error_code ec) { //
      self->m_error = std::move(ec);
    });
  }

  inline void cancel_timer()
  {
    m_timer.cancel(m_error);
  }

  inline void do_read()
  {
    start_timer();
    asio::async_read_until(m_socket, m_buffer, "\r\n\r\n", //
        [self = base_shared_t::shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
          self->m_error = std::move(ec);
          self->on_parse_received_data(bytes_transferred);
        });
  }

  inline void on_parse_received_data(const std::size_t bytes_transferred)
  {
    cancel_timer();

    try
    {
      std::istream stream(&m_buffer);
      read_t received = read_t::parse(stream);
    } catch (...)
    {
      ;
    }
  }

  http_configuration m_configration;
  tcp::socket m_socket;
  asio::steady_timer m_timer;
  asio::streambuf m_buffer;
  boost::system::error_code m_error;
  // role_traits m_role;
};

} // namespace http
