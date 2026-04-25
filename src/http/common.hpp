#pragma once

#include <iosfwd> // std::istream
#include <string>
#include <string_view>
#include <unordered_map>

namespace http
{

/// \brief Supported HTTP request methods.
enum class method
{
  GET,
  POST,
  PUT,
  DELETE_,
  PATCH,
  HEAD,
  OPTIONS,
  UNKNOWN
};

/// \brief Representation of HTTP response code.
enum class status_code : std::uint16_t
{
  OK = 200,
  CREATED = 201,
  NO_CONTENT = 204,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  NOT_MODIFIED = 304,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  NOT_ALLOWED = 405,
  REQUEST_TIMEOUT = 408,
  LARGE_PAYLOAD = 413,
  LARGE_HEADERS = 431,
  INTERNAL_SERVER_ERROR = 500,
  BAD_GATEWAY = 502,
  UNAVAILABLE = 503,
};

/// \brief Converts a string to an HTTP method.
[[nodiscard]] method string_to_method(const std::string_view str);

/// \brief Converts an HTTP method to its string representation.
[[nodiscard]] std::string_view method_to_string(const method m);

/// \brief Returns a standard reason phrase for the given HTTP status code.
[[nodiscard]] std::string_view status_text_for(const status_code code);

/// \brief Type for representation headers in HTTP request/response.
struct headers_t : public std::unordered_map<std::string, std::string>
{
  /// \brief Parsing HTTP headers from the incoming input stream.
  [[nodiscard]] static headers_t parse(std::istream &stream);

  /// \brief Returns value of the "Content-Length" HTTP header.
  [[nodiscard]] std::size_t content_length() const;

  /// \brief   Compares the expected content length from HTTP headers
  ///          with the actual size of bofy for HTTP requests/responses.
  /// \details Used as common part in HTTP requests/responses.
  [[nodiscard]] std::size_t remaining(const std::string &body) const;
};

constexpr inline std::string_view http_version = "HTTP/1.1";

} // namespace http
