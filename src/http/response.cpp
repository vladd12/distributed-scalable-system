#include "http/response.hpp"

#include <core/conversion.hpp>
#include <core/errors.hpp>
#include <sstream>

namespace http
{

status_line status_line::parse(std::istream &stream)
{
  status_line line;
  std::string temp_line;

  // Status line parsing, example: "HTTP/1.1 200 OK"
  if (!std::getline(stream, temp_line))
    throw core::http_error("status line parsing error: empty response");

  if (!temp_line.empty() && temp_line.back() == '\r')
    temp_line.pop_back();

  std::istringstream _status_line(temp_line);
  std::uint16_t code;
  if (!(_status_line >> line.version >> code))
    throw core::http_error("status line parsing error: invalid status line");
  line.status_code = core::from_underlying<::http::status_code>(code);

  return line;
}

std::string response::serialize() const
{
  std::ostringstream oss;
  oss << line.version << ' ' << core::to_underlying(line.status_code) << ' ' //
      << status_text_for(line.status_code) << "\r\n";

  for (const auto &[key, value] : headers)
    oss << key << ": " << value << "\r\n";

  if (!headers.contains("Connection"))
    oss << "Connection: close\r\n";

  if (!headers.contains("Content-Length"))
  {
    // RFC 7230 §3.3.2: 204 must not carry Content-Length. 304 MAY omit it
    // (we do — body is never present). For every other status, send an explicit
    // Content-Length (possibly 0) so the peer knows the body size without
    // relying on connection close — otherwise keep-alive hangs.
    if (line.status_code != status_code::NO_CONTENT && line.status_code != status_code::NOT_MODIFIED)
      oss << "Content-Length: " << body.size() << "\r\n";
  }

  oss << "\r\n";
  if (!body.empty())
    oss << body;

  return oss.str();
}

std::size_t response::remaining() const
{
  return headers.remaining(body);
}

response response::simple(const status_code code)
{
  response resp;
  resp.line.status_code = code;
  resp.line.version = http_version;
  return resp;
}

response response::text(const status_code code, std::string text)
{
  response resp;
  resp.line.status_code = code;
  resp.line.version = http_version;
  resp.headers["Content-Type"] = "text/plain";
  resp.headers["Content-Length"] = std::to_string(text.size());
  resp.body = std::move(text);
  return resp;
}

response response::json(const status_code code, std::string json_body)
{
  response resp;
  resp.line.status_code = code;
  resp.line.version = http_version;
  resp.headers["Content-Type"] = "application/json";
  resp.headers["Content-Length"] = std::to_string(json_body.size());
  resp.body = std::move(json_body);
  return resp;
}

response response::parse(std::istream &stream)
{
  response resp;
  std::string line;

  resp.line = std::move(status_line::parse(stream));  // Status line
  resp.headers = std::move(headers_t::parse(stream)); // Headers

  // Body parsing (rest of stream)
  std::ostringstream body_stream;
  body_stream << stream.rdbuf();
  resp.body = body_stream.str();

  return resp;
}

response response::parse(const std::string &data)
{
  std::istringstream stream(data);
  return response::parse(stream);
}

} // namespace http
