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

} // namespace http
