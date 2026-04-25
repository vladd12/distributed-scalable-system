#include "http/server.hpp"

#include <core/errors.hpp>

namespace http
{

// ---------------------------------------------------------------------------
// http_session
// ---------------------------------------------------------------------------

http_session::pointer http_session::create(tcp::socket socket, request_handler handler)
{
  return std::make_shared<http_session>(std::move(socket), std::move(handler));
}

http_session::http_session(tcp::socket socket, request_handler handler)
    : m_socket(std::move(socket))
    , m_handler(std::move(handler))
    , m_timer(m_socket.get_executor())
    , m_buffer(max_header_size + max_body_size)
    , m_expected_body_size(0)
{
}

void http_session::start()
{
  do_read();
}

void http_session::do_read()
{
  start_timeout();
  auto self = shared_from_this();
  asio::async_read_until(m_socket, m_buffer, "\r\n\r\n", //
      [self](boost::system::error_code ec, std::size_t bytes_transferred) {
        self->on_request_parse(ec, bytes_transferred);
      });
}

void http_session::do_remaining_read(const std::size_t remaining)
{
  if (remaining > max_body_size || (m_request.body.size() + remaining) > max_body_size)
  {
    m_response_data = response::simple(status_code::LARGE_PAYLOAD).serialize();
    do_write();
    return;
  }

  start_timeout();
  auto self = shared_from_this();
  asio::async_read(m_socket, m_buffer, asio::transfer_exactly(remaining), //
      [self](boost::system::error_code ec, std::size_t bytes_transferred) {
        self->on_remaining_data_read(ec, bytes_transferred);
      });
}

void http_session::on_request_parse(boost::system::error_code ec, std::size_t bytes_transferred)
{
  cancel_timeout();

  if (ec == asio::error::operation_aborted)
    return;

  if (ec == asio::error::not_found)
  {
    m_response_data = response::simple(status_code::LARGE_HEADERS).serialize();
    do_write();
    return;
  }

  if (ec)
  {
    shutdown();
    return;
  }

  try
  {
    std::istream stream(&m_buffer);
    m_request = request::parse(stream);

    m_expected_body_size = m_request.headers.content_length();
    if (m_expected_body_size > max_body_size)
    {
      m_response_data = response::simple(status_code::LARGE_PAYLOAD).serialize();
      do_write();
      return;
    }

    if (m_request.body.size() > m_expected_body_size)
      throw core::http_error("request body size exceeds Content-Length");

    const std::size_t remaining = m_expected_body_size - m_request.body.size();
    if (remaining != 0) // need read more data
    {
      do_remaining_read(remaining);
    }
    else // no need read more data
    {
      process_request();
      do_write();
    }
  } catch (const core::http_error &)
  {
    m_response_data = response::text(status_code::BAD_REQUEST, "Bad Request").serialize();
    do_write();
  } catch (const std::exception &)
  {
    m_response_data = response::text(status_code::INTERNAL_SERVER_ERROR, "Internal Server Error").serialize();
    do_write();
  }
}

void http_session::on_remaining_data_read(boost::system::error_code ec, std::size_t bytes_transferred)
{
  cancel_timeout();
  (void)bytes_transferred;

  if (ec == asio::error::operation_aborted)
    return;
  if (ec)
  {
    shutdown();
    return;
  }

  const auto data = m_buffer.data();
  m_request.body.append(asio::buffers_begin(data), asio::buffers_end(data));
  m_buffer.consume(m_buffer.size());

  if (m_request.body.size() != m_expected_body_size)
  {
    m_response_data = response::text(status_code::BAD_REQUEST, "Bad Request").serialize();
    do_write();
    return;
  }
  process_request();
  do_write();
}

void http_session::process_request()
{
  try
  {
    m_response_data = m_handler(m_request).serialize();
  } catch (const core::http_error &)
  {
    m_response_data = response::text(status_code::BAD_REQUEST, "Bad Request").serialize();
  } catch (const std::exception &)
  {
    m_response_data = response::text(status_code::INTERNAL_SERVER_ERROR, "Internal Server Error").serialize();
  }
}

void http_session::do_write()
{
  start_timeout();
  auto self = shared_from_this();
  asio::async_write(m_socket, asio::buffer(m_response_data), [self](boost::system::error_code, std::size_t) {
    self->cancel_timeout();
    self->shutdown();
  });
}

void http_session::start_timeout()
{
  m_timer.expires_after(io_timeout);

  auto self = shared_from_this();
  m_timer.async_wait([self](boost::system::error_code ec) {
    if (!ec)
    {
      self->shutdown();
    }
  });
}

void http_session::cancel_timeout()
{
  boost::system::error_code timer_ec;
  m_timer.cancel(timer_ec);
}

void http_session::shutdown()
{
  cancel_timeout();

  if (!m_socket.is_open())
    return;

  boost::system::error_code cancel_ec;
  m_socket.cancel(cancel_ec);

  boost::system::error_code shutdown_ec;
  m_socket.shutdown(tcp::socket::shutdown_both, shutdown_ec);

  boost::system::error_code close_ec;
  m_socket.close(close_ec);
}

// ---------------------------------------------------------------------------
// http_server
// ---------------------------------------------------------------------------

http_server::http_server(asio::io_context &ctx, const std::uint16_t port)
    : m_acceptor(ctx, tcp::endpoint(asio::ip::make_address("0.0.0.0"), port))
{
  m_default_handler = [](const request &) { return response::text(status_code::NOT_FOUND, "Not Found"); };
  do_accept();
}

void http_server::route(method m, const std::string &path, request_handler handler)
{
  m_routes[route_key { m, path }] = std::move(handler);
}

void http_server::set_default_handler(request_handler handler)
{
  m_default_handler = std::move(handler);
}

void http_server::do_accept()
{
  m_acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
    if (ec == asio::error::operation_aborted)
      return;
    if (!ec)
    {
      auto handler = [this](const request &req) { return dispatch(req); };
      http_session::create(std::move(socket), handler)->start();
    }
    do_accept();
  });
}

response http_server::dispatch(const request &req)
{
  auto it = m_routes.find(route_key { req.line.method_type, req.line.path });
  if (it != m_routes.end())
    return it->second(req);
  return m_default_handler(req);
}

std::size_t http_server::route_key_hash::operator()(const route_key &key) const
{
  auto h1 = std::hash<int> {}(static_cast<int>(key.method_type));
  auto h2 = std::hash<std::string> {}(key.path);
  return h1 ^ (h2 << 1);
}

} // namespace http
