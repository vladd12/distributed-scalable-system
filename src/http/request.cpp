#include "http/request.hpp"

#include <algorithm>
#include <core/errors.hpp>
#include <sstream>

namespace http
{

std::string request::serialize() const
{
  std::ostringstream req;
  req << method_to_string(method_type) << ' ' << path << ' ' << version << "\r\n";

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

request request::parse(std::istream &stream)
{
  request req;
  std::string line;

  // Request line: METHOD PATH VERSION
  if (!std::getline(stream, line))
    throw core::http_error("response parsing error: empty response");

  if (!line.empty() && line.back() == '\r')
    line.pop_back();

  std::istringstream request_line(line);
  std::string method_str;
  if (!(request_line >> method_str >> req.path >> req.version))
    throw core::http_error("response parsing error: invalid status line");

  req.method_type = string_to_method(method_str);

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

    req.headers[std::move(key)] = std::move(value);
  }

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
