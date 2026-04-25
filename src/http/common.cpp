#include "http/common.hpp"

#include <algorithm>
#include <charconv>
#include <core/errors.hpp>
#include <istream>

namespace http
{

method string_to_method(const std::string_view str)
{
  if (str == "GET")
    return method::GET;
  if (str == "POST")
    return method::POST;
  if (str == "PUT")
    return method::PUT;
  if (str == "DELETE")
    return method::DELETE_;
  if (str == "PATCH")
    return method::PATCH;
  if (str == "HEAD")
    return method::HEAD;
  if (str == "OPTIONS")
    return method::OPTIONS;
  return method::UNKNOWN;
}

std::string_view method_to_string(const method m)
{
  switch (m)
  {
  case method::GET:
    return "GET";
  case method::POST:
    return "POST";
  case method::PUT:
    return "PUT";
  case method::DELETE_:
    return "DELETE";
  case method::PATCH:
    return "PATCH";
  case method::HEAD:
    return "HEAD";
  case method::OPTIONS:
    return "OPTIONS";
  default:
    return "UNKNOWN";
  }
}

std::string_view status_text_for(const status_code code)
{
  switch (code)
  {
  case status_code::OK:
    return "OK";
  case status_code::CREATED:
    return "Created";
  case status_code::NO_CONTENT:
    return "No Content";
  case status_code::MOVED_PERMANENTLY:
    return "Moved Permanently";
  case status_code::FOUND:
    return "Found";
  case status_code::NOT_MODIFIED:
    return "Not Modified";
  case status_code::BAD_REQUEST:
    return "Bad Request";
  case status_code::UNAUTHORIZED:
    return "Unauthorized";
  case status_code::FORBIDDEN:
    return "Forbidden";
  case status_code::NOT_FOUND:
    return "Not Found";
  case status_code::NOT_ALLOWED:
    return "Method Not Allowed";
  case status_code::REQUEST_TIMEOUT:
    return "Request Timeout";
  case status_code::LARGE_PAYLOAD:
    return "Payload Too Large";
  case status_code::LARGE_HEADERS:
    return "Request Header Fields Too Large";
  case status_code::INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case status_code::BAD_GATEWAY:
    return "Bad Gateway";
  case status_code::UNAVAILABLE:
    return "Service Unavailable";
  default:
    return "Unknown";
  }
}

headers_t headers_t::parse(std::istream &stream)
{
  headers_t headers;
  std::string line;

  // Headers parsing
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

    headers[std::move(key)] = std::move(value);
  }

  return headers;
}

std::size_t headers_t::content_length() const
{
  const auto find_iter = find("content-length");
  if (find_iter != cend())
  {
    const auto &value = find_iter->second;
    if (value.empty())
      throw core::http_error("request parsing error: invalid 'Content-Length' header");

    std::size_t content_length = 0;
    const char *begin = value.data();
    const char *end = value.data() + value.size();
    const auto [parse_end, parse_error] = std::from_chars(begin, end, content_length);
    if (parse_error != std::errc() || parse_end != end)
      throw core::http_error("request parsing error: invalid 'Content-Length' header");

    return content_length;
  }
  else
    return 0;
}

std::size_t headers_t::remaining(const std::string &body) const
{
  const std::size_t expected = content_length();
  const std::size_t actual = body.length();
  if (actual < expected)
    return expected - actual;
  else
    return 0;
}

} // namespace http
