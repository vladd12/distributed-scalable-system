#pragma once

#include <http/common.hpp>
#include <http/headers.hpp>

namespace http
{
/// @brief   Parsed status line of an HTTP response.
/// @details Wire format: `HTTP-version SP status-code SP reason-phrase CRLF`.
///          The reason phrase is not stored — status_text_for(status_code)
///          derives it on demand during serialization.
struct status_line
{
  status_code status_code = status_code::OK; ///< Numeric status code.
  std::string version;                       ///< HTTP version token (e.g. "HTTP/1.1").

  /// @brief  Parses a status line from `stream`.
  /// @throws core::http_error on empty stream or malformed line.
  [[nodiscard]] static status_line parse(std::istream &stream);
};

/// @brief   HTTP response (both parsed and to-be-serialized).
/// @details Wire format:
///          ```
///          status-line CRLF
///          header-field CRLF
///          ...
///          CRLF
///          message-body
///          ```
struct response
{
  status_line line;  ///< Status line (code, version).
  headers_t headers; ///< Normalized headers (lowercase keys).
  std::string body;  ///< Message body (may be empty).

  /// @brief   Serializes the response into HTTP/1.1 wire format.
  /// @details Adds `Content-Length` if absent (based on `body.size()`).
  ///          Adds `Connection: close` if absent.
  /// @return  Full response bytes (headers + CRLF + body).
  [[nodiscard]] std::string serialize() const;

  /// @brief   Returns size of remaining data for reading from incoming HTTP response.
  /// @details Used in HTTP client. Equivalent to headers.remaining(body).
  [[nodiscard]] std::size_t remaining() const;

  /// @brief  Creates an empty response with the given status code.
  /// @param  code Status code to use.
  /// @return Response with `line.version = http_version` and empty body/headers.
  [[nodiscard]] static response simple(const status_code code);

  /// @brief Creates a plain-text response (`Content-Type: text/plain`).
  /// @param code Status code to use.
  /// @param text Body content (copied).
  [[nodiscard]] static response text(const status_code code, const std::string_view &text);

  /// @brief Creates a JSON response (`Content-Type: application/json`).
  /// @param code      Status code to use.
  /// @param json_body Serialized JSON (copied into `body`).
  [[nodiscard]] static response json(const status_code code, const std::string_view &json_body);

  /// @brief  Parses a raw HTTP response from an input stream.
  /// @throws core::http_error on malformed status line or headers.
  [[nodiscard]] static response parse(std::istream &stream);

  /// @brief  Parses a raw HTTP response from a string (for testing or simple usage).
  /// @throws core::http_error on malformed input.
  [[nodiscard]] static response parse(const std::string &data);
};

} // namespace http
