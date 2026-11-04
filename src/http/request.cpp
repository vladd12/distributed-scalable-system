#include "http/request.hpp"

#include <algorithm>
#include <core/errors.hpp>
#include <sstream>

namespace http
{

request_line request_line::parse(std::istream &stream)
{
  request_line line;
  std::string temp_line;

  // Request line: METHOD PATH VERSION
  if (!std::getline(stream, temp_line))
    throw core::http_error("response parsing error: empty response");

  if (!temp_line.empty() && temp_line.back() == '\r')
    temp_line.pop_back();

  std::istringstream request_line(temp_line);
  std::string method_str;
  if (!(request_line >> method_str >> line.path >> line.version))
    throw core::http_error("response parsing error: invalid status line");

  line.method_type = string_to_method(method_str);
  return line;
}

std::string request::serialize() const
{
  std::ostringstream req;
  req << method_to_string(line.method_type) << ' ' << line.path << ' ' << line.version << "\r\n";

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
