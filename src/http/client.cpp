#include "http/client.hpp"

#include <sstream>

namespace http
{

// ---------------------------------------------------------------------------
// client_session
// ---------------------------------------------------------------------------

client_session::pointer client_session::create(asio::io_context &io_context)
{
  return client_session::pointer(new client_session(io_context));
}

client_session::client_session(asio::io_context &io_context) : m_socket(io_context), m_resolver(io_context)
{
}

void client_session::connect(const std::string &host, const std::uint16_t port)
{
  auto self = shared_from_this();
  m_resolver.async_resolve(host, std::to_string(port), //
      [self](boost::system::error_code ec, tcp::resolver::results_type results) {
        self->on_resolve(ec, results); //
      });
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
  auto self = shared_from_this();
  asio::async_write(m_socket, asio::buffer(req.serialize()),
      [self](boost::system::error_code ec, std::size_t bytes) { self->on_write_request(ec, bytes); });
}

void client_session::on_write_request(boost::system::error_code ec, std::size_t /*bytes_transferred*/)
{
  if (ec)
  {
    finish(ec);
    return;
  }
  do_read_headers();
}

void client_session::do_read_headers()
{
  auto self = shared_from_this();
  asio::async_read_until(
      m_socket, m_buffer, "\r\n\r\n", [self](boost::system::error_code ec, std::size_t bytes_transferred) {
        self->on_headers_read(ec, bytes_transferred);
      });
}

void client_session::on_headers_read(boost::system::error_code ec, std::size_t /*bytes*/)
{
  if (ec)
  {
    finish(ec);
    return;
  }

  // Extract headers from buffer
  std::istream stream(&m_buffer);
  std::string line;
  std::string headers_str;
  while (std::getline(stream, line))
  {
    headers_str += line + "\n";
    if (line == "\r" || line.empty())
      break;
  }

  m_response = std::move(response::parse(headers_str));
  /// TODO: must be try-catch construction here to avoid errors due parsing response
  /*
  if (!m_response.parse(headers_str))
  {
    finish(boost::asio::error::invalid_argument);
    return;
  }
  */

  // Determine content length
  auto it = m_response.headers.find("content-length");
  if (it != m_response.headers.end())
  {
    m_content_length = std::stoul(it->second);
    m_keep_alive = true; // assume keep-alive if content-length present
  }
  else
  {
    // No content-length, read until connection close
    m_content_length = 0;
    m_keep_alive = false;
  }

  // Check if we already have some body data in the buffer
  std::size_t buffered = m_buffer.size();
  if (buffered > 0)
  {
    std::ostringstream body_buf;
    body_buf << stream.rdbuf(); // read remaining buffered data
    m_response.body = body_buf.str();
  }

  if (m_content_length > 0 && m_response.body.size() < m_content_length)
  {
    // Need to read more body
    do_read_body();
  }
  else
  {
    // Body complete or none expected
    finish(boost::system::error_code());
  }
}

void client_session::do_read_body()
{
  auto self = shared_from_this();
  std::size_t remaining = m_content_length - m_response.body.size();
  asio::async_read(m_socket, m_buffer, asio::transfer_exactly(remaining),
      [self](boost::system::error_code ec, std::size_t bytes) { self->on_body_read(ec, bytes); });
}

void client_session::on_body_read(boost::system::error_code ec, std::size_t /*bytes_transferred*/)
{
  if (ec)
  {
    finish(ec);
    return;
  }

  // Extract remaining body from buffer
  std::istream stream(&m_buffer);
  std::ostringstream body_buf;
  body_buf << stream.rdbuf();
  m_response.body += body_buf.str();

  finish(boost::system::error_code());
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
    : m_ctx(ctx), m_host(host), m_session(client_session::create(ctx))
{
  m_session->connect(host, port);
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
  m_session->write(req, std::move(handler));
}

} // namespace http
