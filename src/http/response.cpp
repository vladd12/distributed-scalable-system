#include "http/response.hpp"

#include <algorithm>
#include <core/errors.hpp>
#include <sstream>

namespace http
{

std::string response::serialize() const
{
  std::ostringstream oss;
  oss << version << ' ' << status_code << ' ' << status_text_for(status_code) << "\r\n";

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

response response::text(unsigned int code, const std::string_view &text)
{
  response resp;
  resp.status_code = code;
  resp.version = "HTTP/1.1";
  resp.headers["Content-Type"] = "text/plain";
  resp.body = text;
  return resp;
}

response response::json(unsigned int code, const std::string_view &json_body)
{
  response resp;
  resp.status_code = code;
  resp.version = "HTTP/1.1";
  resp.headers["Content-Type"] = "application/json";
  resp.body = json_body;
  return resp;
}

response response::parse(std::istream &stream)
{
  response resp;
  std::string line;

  // Status line parsing, example: "HTTP/1.1 200 OK"
  if (!std::getline(stream, line))
    throw core::http_error("response parsing error: empty response");

  if (!line.empty() && line.back() == '\r')
    line.pop_back();
  std::istringstream status_line(line);
  if (!(status_line >> resp.version >> resp.status_code))
    throw core::http_error("response parsing error: invalid status line");

  // Headers parsing, example: "key: value"
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

    // Lowercase key for case-insensitive lookup
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });

    // Trim leading whitespace from value
    if (auto pos = value.find_first_not_of(' '); pos != std::string::npos)
      value = value.substr(pos);

    resp.headers[std::move(key)] = std::move(value);
  }

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
