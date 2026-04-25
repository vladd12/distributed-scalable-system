#include "http/client.hpp"

constexpr inline bool use_async = false;

namespace http
{

// ---------------------------------------------------------------------------
// client_session
// ---------------------------------------------------------------------------

client_session::pointer client_session::create(asio::io_context &io_context)
{
  return std::make_shared<client_session>(io_context);
}

client_session::client_session(asio::io_context &io_context) : m_socket(io_context), m_resolver(io_context)
{
}

void client_session::async_connect(const std::string &host, const std::uint16_t port)
{
  auto self = shared_from_this();
  m_resolver.async_resolve(host, std::to_string(port), //
      [self](boost::system::error_code ec, tcp::resolver::results_type results) {
        self->on_resolve(ec, results); //
      });
}

void client_session::sync_connect(const std::string &host, const std::uint16_t port)
{
  boost::system::error_code err;
  tcp::resolver::results_type results = m_resolver.resolve(host, std::to_string(port), err);
  if (!err)
    asio::connect(m_socket, results, err);

  if (err)
  {
    finish(err);
    return;
  }
}

void client_session::on_resolve(boost::system::error_code ec, tcp::resolver::results_type results)
{
  if (ec)
  {
    finish(ec);
    return;
  }
  do_connect(results);
}

void client_session::do_connect(tcp::resolver::results_type results)
{
  auto self = shared_from_this();
  asio::async_connect(m_socket, results, [self](boost::system::error_code ec, tcp::endpoint) { self->on_connect(ec); });
}

void client_session::on_connect(boost::system::error_code ec)
{
  if (ec)
  {
    finish(ec);
    return;
  }
}

void client_session::write(const request &req, response_callback handler)
{
  m_handler = std::move(handler); /// TODO: session works only with one request... looks bad
  m_request = std::move(req.serialize());
  asio::const_buffer request_buffer(m_request.data(), m_request.size());
  auto self = shared_from_this();
  asio::async_write(m_socket, request_buffer,
      [self](boost::system::error_code ec, std::size_t bytes) { self->on_write_request(ec, bytes); });
}

void client_session::on_write_request(boost::system::error_code ec, std::size_t bytes_transferred)
{
  if (ec)
  {
    finish(ec);
    return;
  }
  do_read();
}

void client_session::do_read()
{
  auto self = shared_from_this();
  asio::async_read_until(m_socket, m_buffer, "\r\n\r\n", //
      [self](boost::system::error_code ec, std::size_t bytes_transferred) {
        self->on_response_parse(ec, bytes_transferred);
      });
}

void client_session::do_remaining_read(const std::size_t remaining)
{
  auto self = shared_from_this();
  asio::async_read(m_socket, m_buffer, asio::transfer_exactly(remaining), //
      [self](boost::system::error_code ec, std::size_t bytes_transferred) {
        self->on_remaining_data_read(ec, bytes_transferred);
      });
}

void client_session::on_response_parse(boost::system::error_code ec, std::size_t bytes_transferred)
{
  if (ec && ec != asio::error::eof)
  {
    finish(ec);
    return;
  }

  try
  {
    std::istream stream(&m_buffer);
    m_response = std::move(response::parse(stream));

    // content length request body diff check
    const std::size_t remaining = m_response.remaining();
    // need reed more data
    if (remaining != 0)
      do_remaining_read(remaining);
    // no need reed more data
    else
      finish(boost::system::error_code()); // no error
  } catch (...)
  {
    finish(asio::error::invalid_argument);
  }
}

void client_session::on_remaining_data_read(boost::system::error_code ec, std::size_t bytes_transferred)
{
  if (ec && ec != asio::error::eof)
  {
    finish(ec);
    return;
  }

  std::string_view view(static_cast<std::string_view::const_pointer>(m_buffer.data().data()), m_buffer.data().size());
  m_response.body += view;
  finish(boost::system::error_code()); // no error
}

void client_session::finish(boost::system::error_code ec)
{
  if (m_handler)
  {
    m_handler(ec, std::move(m_response));
  }
  // socket will be closed automatically when session is destroyed
}

// ---------------------------------------------------------------------------
// http_client
// ---------------------------------------------------------------------------

http_client::http_client(asio::io_context &ctx, const std::string &host, const std::uint16_t port)
    : m_host(host), m_session(client_session::create(ctx))
{
  if constexpr (use_async)
    m_session->async_connect(host, port);
  else
    m_session->sync_connect(host, port);
}

void http_client::get(const std::string &path, const headers_t &headers, response_callback handler)
{
  request req { method::GET, path, "HTTP/1.1", headers };
  async_request(std::move(req), std::move(handler));
}

void http_client::post(
    const std::string &path, const headers_t &headers, const std::string &body, response_callback handler)
{
  request req { method::POST, path, "HTTP/1.1", headers, body };
  async_request(std::move(req), std::move(handler));
}

void http_client::put(
    const std::string &path, const headers_t &headers, const std::string &body, response_callback handler)
{
  request req { method::PUT, path, "HTTP/1.1", headers, body };
  async_request(std::move(req), std::move(handler));
}

void http_client::delete_(const std::string &path, const headers_t &headers, response_callback handler)
{
  request req { method::DELETE_, path, "HTTP/1.1", headers };
  async_request(std::move(req), std::move(handler));
}

void http_client::async_request(request req, response_callback handler)
{
  req.headers["Host"] = m_host;
  req.headers["Connection"] = "Keep-Alive"; // HTTP 1.1 specific
  if (!req.body.empty())
    req.headers["Content-Length"] = std::to_string(req.body.length());
  m_session->write(req, std::move(handler));
}

} // namespace http
