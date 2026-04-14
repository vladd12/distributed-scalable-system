#include "http/response.hpp"

#include <algorithm>
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
    throw core::http_error("response parsing error: empty response");

  if (!temp_line.empty() && temp_line.back() == '\r')
    temp_line.pop_back();

  std::istringstream _status_line(temp_line);
  if (!(_status_line >> line.version >> line.status_code))
    throw core::http_error("response parsing error: invalid status line");

  return line;
}

std::string response::serialize() const
{
  std::ostringstream oss;
  oss << line.version << ' ' << line.status_code << ' ' << status_text_for(line.status_code) << "\r\n";

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
  resp.line.status_code = code;
  resp.line.version = http_version;
  resp.headers["Content-Type"] = "text/plain";
  resp.body = text;
  return resp;
}

response response::json(unsigned int code, const std::string_view &json_body)
{
  response resp;
  resp.line.status_code = code;
  resp.line.version = http_version;
  resp.headers["Content-Type"] = "application/json";
  resp.body = json_body;
  return resp;
}

response response::parse(std::istream &stream)
{
  response resp;
  std::string line;

  resp.line = std::move(status_line::parse(stream)); // Status line
  resp.headers = std::move(parse_headers(stream));   // Headers

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
