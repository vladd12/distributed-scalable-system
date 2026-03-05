#include "http/response.hpp"

#include <sstream>

namespace http
{

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

} // namespace http
