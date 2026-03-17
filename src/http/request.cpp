#include "http/request.hpp"

#include <algorithm>
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

} // namespace http
