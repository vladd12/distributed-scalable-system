#include "http/common.hpp"

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

} // namespace http
