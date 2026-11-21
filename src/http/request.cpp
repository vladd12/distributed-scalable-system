#include "http/request.hpp"

#include <core/errors.hpp>
#include <sstream>
#include <utils/strings.hpp>

namespace http
{

request_line request_line::parse(std::istream &stream)
{
  request_line line;
  std::string temp_line;

  // Request line: METHOD PATH VERSION
  if (!std::getline(stream, temp_line))
    throw core::http_error("request line parsing error: empty request");

  if (!temp_line.empty() && temp_line.back() == '\r')
    temp_line.pop_back();

  std::istringstream request_line(temp_line);
  std::string method_str;
  if (!(request_line >> method_str >> line.path >> line.version))
    throw core::http_error("request line parsing error: invalid request line");

  line.method_type = string_to_method(method_str);
  return line;
}

std::string request::serialize() const
{
  // RFC 7230 §5.4: Host is mandatory in HTTP/1.1 requests.
  if (!headers.contains("Host"))
    throw core::http_error("request::serialize: 'Host' header is required in HTTP/1.1");

  std::ostringstream req;
  req << method_to_string(line.method_type) << ' ' << line.path << ' ' << line.version << "\r\n";

  for (const auto &[key, value] : headers)
    req << key << ": " << value << "\r\n";

  if (!headers.contains("Connection"))
    req << utils::to_lower("Connection: ") << "close\r\n";

  if (!headers.contains("Content-Length"))
  {
    // RFC 7230 §3.3.2: methods that anticipate a payload body SHOULD carry an
    // explicit Content-Length (possibly 0), even for an empty body.
    const bool is_method_anticipates_body
        = (line.method_type == method::POST || line.method_type == method::PUT || line.method_type == method::PATCH);
    if (is_method_anticipates_body || !body.empty())
      req << utils::to_lower("Content-Length: ") << body.size() << "\r\n";
  }

  req << "\r\n";
  if (!body.empty())
    req << body;

  return req.str();
}

std::size_t request::remaining() const
{
  return headers.remaining(body);
}

request request::parse(std::istream &stream)
{
  request req;
  std::string line;

  req.line = std::move(request_line::parse(stream)); // Request line
  req.headers = std::move(headers_t::parse(stream)); // Headers

  // Body parsing (rest of stream)
  std::ostringstream body_stream;
  body_stream << stream.rdbuf();
  req.body = body_stream.str();

  return req;
}

request request::parse(const std::string &data)
{
  std::istringstream stream(data);
  return request::parse(stream);
}

} // namespace http
