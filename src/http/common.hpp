#pragma once

#include <chrono>
#include <string_view>

namespace http
{

/// @brief Supported HTTP request methods.
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

/// @brief Representation of HTTP response code.
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

/// @brief   Common HTTP session configuration.
/// @details Used by both client and server sessions.
struct http_configuration
{
  std::chrono::milliseconds io_timeout { 5'000 }; ///< Per-operation timeout
  std::size_t max_header_size = 16 * 1024;        ///< Max bytes before CRLF CRLF
  std::size_t max_body_size = 1024 * 1024;        ///< Max body bytes (Content-Length)
};

/// @brief  Converts a string to an HTTP method.
/// @return Matching method, or method::UNKNOWN if `str` is not a known method name.
[[nodiscard]] method string_to_method(const std::string_view str);

/// @brief Converts an HTTP method to its string representation.
[[nodiscard]] std::string_view method_to_string(const method m);

/// @brief Returns a standard reason phrase for the given HTTP status code.
[[nodiscard]] std::string_view status_text_for(const status_code code);

/// @brief HTTP version string used in serialized messages.
constexpr inline std::string_view http_version = "HTTP/1.1";

} // namespace http
