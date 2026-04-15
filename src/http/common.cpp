#include "http/common.hpp"

#include <algorithm>
#include <sstream>

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

std::string_view status_text_for(const unsigned int code)
{
  switch (code)
  {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 304:
    return "Not Modified";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 500:
    return "Internal Server Error";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  default:
    return "Unknown";
  }
}

headers_t parse_headers(std::istream &stream)
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

std::size_t get_remaining_data_length(const headers_t &headers, const std::string &body) noexcept
{
  const auto find_iter = headers.find("content-length");
  if (find_iter != headers.cend())
  {
    const std::size_t expected_content_length = std::stoull(find_iter->second);
    const std::size_t actual_content_length = body.length();
    if (actual_content_length < expected_content_length)
      return expected_content_length - actual_content_length;
  }
  return 0;
}

} // namespace http
