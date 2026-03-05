#include "http/server.hpp"

#include <algorithm>
#include <sstream>

namespace http
{

// ---------------------------------------------------------------------------
// response
// ---------------------------------------------------------------------------

std::string response::serialize() const
{
  std::ostringstream oss;
  oss << "HTTP/1.1 " << status_code << ' ' << status_text_for(status_code) << "\r\n";

  bool has_content_length = false;
  bool has_connection = false;
  for (const auto &[key, value] : headers)
  {
    oss << key << ": " << value << "\r\n";
    if (key == "Content-Length")
      has_content_length = true;
    if (key == "Connection")
      has_connection = true;
  }

  if (!has_content_length)
    oss << "Content-Length: " << body.size() << "\r\n";
  if (!has_connection)
    oss << "Connection: close\r\n";

  oss << "\r\n" << body;
  return oss.str();
}

response response::text(unsigned int code, std::string_view text)
{
  response resp;
  resp.status_code = code;
  resp.headers["Content-Type"] = "text/plain";
  resp.body = text;
  return resp;
}

response response::json(unsigned int code, std::string_view json_body)
{
  response resp;
  resp.status_code = code;
  resp.headers["Content-Type"] = "application/json";
  resp.body = json_body;
  return resp;
}

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
  asio::async_read_until(
      m_socket, m_buffer, "\r\n\r\n", [self](boost::system::error_code ec, std::size_t bytes_transferred) {
        self->on_headers_read(ec, bytes_transferred);
      });
}

void http_session::on_headers_read(boost::system::error_code ec, std::size_t /*bytes_transferred*/)
{
  if (ec)
    return;

  if (!parse_request())
  {
    m_response_data = response::text(400, "Bad Request").serialize();
    do_write();
    return;
  }

  // Check whether a request body is expected.
  auto it = m_request.headers.find("content-length");
  if (it != m_request.headers.end())
  {
    std::size_t content_length = std::stoul(it->second);
    std::size_t already_buffered = m_buffer.size();

    if (already_buffered >= content_length)
    {
      std::istream stream(&m_buffer);
      m_request.body.resize(content_length);
      stream.read(m_request.body.data(), static_cast<std::streamsize>(content_length));
      process_request();
    }
    else
    {
      std::size_t remaining = content_length - already_buffered;
      auto self = shared_from_this();
      asio::async_read(m_socket, m_buffer, asio::transfer_exactly(remaining),
          [self, content_length](boost::system::error_code ec, std::size_t) {
            if (ec)
              return;
            std::istream stream(&self->m_buffer);
            self->m_request.body.resize(content_length);
            stream.read(self->m_request.body.data(), static_cast<std::streamsize>(content_length));
            self->process_request();
          });
    }
  }
  else
  {
    process_request();
  }
}

void http_session::process_request()
{
  m_response_data = m_handler(m_request).serialize();
  do_write();
}

bool http_session::parse_request()
{
  std::istream stream(&m_buffer);
  std::string line;

  // Request line: METHOD PATH VERSION
  if (!std::getline(stream, line))
    return false;
  if (!line.empty() && line.back() == '\r')
    line.pop_back();

  std::istringstream request_line(line);
  std::string method_str, path, version;
  if (!(request_line >> method_str >> path >> version))
    return false;

  m_request.method_type = string_to_method(method_str);
  m_request.path = std::move(path);
  m_request.version = std::move(version);

  // Headers
  while (std::getline(stream, line))
  {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.empty())
      break;

    auto colon = line.find(':');
    if (colon == std::string::npos)
      continue;

    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 1);

    // Lowercase the key for case-insensitive lookup
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });

    // Trim leading whitespace from value
    if (auto pos = value.find_first_not_of(' '); pos != std::string::npos)
      value = value.substr(pos);

    m_request.headers[std::move(key)] = std::move(value);
  }

  return true;
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

http_server::http_server(asio::io_context &ctx, const std::string &address, std::uint16_t port)
    : m_ctx(ctx), m_acceptor(ctx, tcp::endpoint(asio::ip::make_address(address), port))
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
