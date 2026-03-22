#include "http/server.hpp"

namespace http
{

// ---------------------------------------------------------------------------
// http_session
// ---------------------------------------------------------------------------

http_session::pointer http_session::create(tcp::socket socket, request_handler handler)
{
  return pointer(new http_session(std::move(socket), std::move(handler)));
}

http_session::http_session(tcp::socket socket, request_handler handler)
    : m_socket(std::move(socket)), m_handler(std::move(handler))
{
}

void http_session::start()
{
  do_read();
}

void http_session::do_read()
{
  auto self = shared_from_this();
  asio::async_read(
      m_socket, m_buffer, asio::transfer_all(), [self](boost::system::error_code ec, std::size_t bytes_transferred) {
        self->on_request_parse(ec, bytes_transferred);
      });
}

void http_session::on_request_parse(boost::system::error_code ec, std::size_t bytes_transferred)
{
  if (ec && ec != asio::error::eof)
    return;

  try
  {
    std::istream stream(&m_buffer);
    m_request = std::move(request::parse(stream));
    process_request();
  } catch (...) /// TODO: realize that we must catch defferent errors from different parts of application
  {
    m_response_data = response::text(400, "Bad Request").serialize();
  }
  do_write();
}

void http_session::process_request()
{
  try
  {
    m_response_data = std::move(m_handler(m_request).serialize());
  } catch (...) /// TODO: Logic exceptions must be here
  {
    m_response_data = response::text(500, "Internal Server Error").serialize();
  }
}

void http_session::do_write()
{
  auto self = shared_from_this();
  asio::async_write(m_socket, asio::buffer(m_response_data), [self](boost::system::error_code ec, std::size_t) {
    if (!ec)
    {
      boost::system::error_code shutdown_ec;
      self->m_socket.shutdown(tcp::socket::shutdown_both, shutdown_ec);
    }
  });
}

// ---------------------------------------------------------------------------
// http_server
// ---------------------------------------------------------------------------

http_server::http_server(asio::io_context &ctx, const std::uint16_t port)
    : m_acceptor(ctx, tcp::endpoint(asio::ip::make_address("0.0.0.0"), port))
{
  m_default_handler = [](const request &) { return response::text(404, "Not Found"); };
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
  auto it = m_routes.find(route_key { req.method_type, req.path });
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
