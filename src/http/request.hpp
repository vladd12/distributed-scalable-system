#pragma once

#include <http/common.hpp>

namespace http
{

/// @brief   Parsed request line of an HTTP request.
/// @details Wire format: `METHOD SP request-target SP HTTP-version CRLF`.
struct request_line
{
  method method_type = method::UNKNOWN; ///< Parsed method, or UNKNOWN if unrecognized.
  std::string path;                     ///< Request target (path + optional query).
  std::string version;                  ///< HTTP version token (e.g. "HTTP/1.1").

  /// @brief  Parses a request line from `stream`.
  /// @throws core::http_error on empty stream or malformed line.
  [[nodiscard]] static request_line parse(std::istream &stream);
};

/// @brief   Parsed HTTP request.
/// @details Wire format:
///          ```
///          request-line CRLF
///          header-field CRLF
///          ...
///          CRLF
///          message-body
///          ```
struct request
{
  request_line line; ///< Request line (method, target, version).
  headers_t headers; ///< Normalized headers (lowercase keys).
  std::string body;  ///< Message body (may be empty).

  /// @brief   Serializes the request into HTTP/1.1 wire format.
  /// @details Adds `Connection: close` if no Connection header is present.
  ///          Adds `Content-Length` if the body is non-empty and no such header exists.
  /// @return  Full request bytes (headers + CRLF + body).
  [[nodiscard]] std::string serialize() const;

  /// @brief   Returns size of remaining data for reading from incoming HTTP request.
  /// @details Used in HTTP server. Equivalent to headers.remaining(body).
  [[nodiscard]] std::size_t remaining() const;

  /// @brief  Parses a raw HTTP request from an input stream.
  /// @throws core::http_error on malformed request line or headers.
  [[nodiscard]] static request parse(std::istream &stream);

  /// @brief  Parses a raw HTTP request from a string (for testing or simple usage).
  /// @throws core::http_error on malformed input.
  [[nodiscard]] static request parse(const std::string &data);
};

} // namespace http
