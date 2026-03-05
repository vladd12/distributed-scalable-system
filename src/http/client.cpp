#include "http/client.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace http
{

// ---------------------------------------------------------------------------
// client_session
// ---------------------------------------------------------------------------

client_session::pointer client_session::create(asio::io_context &io_context, const std::string &host,
    const std::string &port, const std::string &req, response_callback handler)
{
  return pointer(new client_session(io_context, host, port, req, std::move(handler)));
}

client_session::client_session(asio::io_context &io_context, const std::string &host, const std::string &port,
    const std::string &req, response_callback handler)
    : m_socket(io_context)
    , m_resolver(io_context)
    , m_host(host)
    , m_port(port)
    , m_request_data(req)
    , m_handler(std::move(handler))
{
}

void client_session::start()
{
  do_resolve();
}

void client_session::do_resolve()
{
  auto self = shared_from_this();
  m_resolver.async_resolve(m_host, m_port,
      [self](boost::system::error_code ec, tcp::resolver::results_type results) { self->on_resolve(ec, results); });
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
  do_write_request();
}

void client_session::do_write_request()
{
  auto self = shared_from_this();
  asio::async_write(m_socket, asio::buffer(m_request_data),
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

http_client::http_client(asio::io_context &ctx) : m_ctx(ctx)
{
}

void http_client::async_get(const std::string &host, const std::string &port, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, response_callback handler)
{
  async_request(method::GET, host, port, path, headers, "", std::move(handler));
}

void http_client::async_post(const std::string &host, const std::string &port, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, const std::string &body, response_callback handler)
{
  async_request(method::POST, host, port, path, headers, body, std::move(handler));
}

void http_client::async_put(const std::string &host, const std::string &port, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, const std::string &body, response_callback handler)
{
  async_request(method::PUT, host, port, path, headers, body, std::move(handler));
}

void http_client::async_delete(const std::string &host, const std::string &port, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, response_callback handler)
{
  async_request(method::DELETE_, host, port, path, headers, "", std::move(handler));
}

void http_client::async_request(method m, const std::string &host, const std::string &port, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, const std::string &body, response_callback handler)
{
  std::string request_data = build_request(m, host, path, headers, body);
  auto session = client_session::create(m_ctx, host, port, request_data, std::move(handler));
  session->start();
}

std::string http_client::build_request(method m, const std::string &host, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, const std::string &body) const
{
  std::ostringstream req;
  req << method_to_string(m) << ' ' << path << " HTTP/1.1\r\n";
  req << "Host: " << host << "\r\n";

  bool has_connection = false;
  bool has_content_length = false;
  for (const auto &[key, value] : headers)
  {
    req << key << ": " << value << "\r\n";
    std::string lkey = key;
    std::transform(lkey.begin(), lkey.end(), lkey.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lkey == "connection")
      has_connection = true;
    if (lkey == "content-length")
      has_content_length = true;
  }

  if (!has_connection)
    req << "Connection: close\r\n"; // Simple: close after each request

  if (!has_content_length && !body.empty())
    req << "Content-Length: " << body.size() << "\r\n";

  req << "\r\n";
  if (!body.empty())
    req << body;

  return req.str();
}

} // namespace http
