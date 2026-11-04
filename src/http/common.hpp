#pragma once

#include <iosfwd> // std::istream
#include <string>
#include <string_view>
#include <unordered_map>

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

/// @brief  Converts a string to an HTTP method.
/// @return Matching method, or method::UNKNOWN if `str` is not a known method name.
[[nodiscard]] method string_to_method(const std::string_view str);

/// @brief Converts an HTTP method to its string representation.
[[nodiscard]] std::string_view method_to_string(const method m);

/// @brief Returns a standard reason phrase for the given HTTP status code.
[[nodiscard]] std::string_view status_text_for(const status_code code);

/// @brief   HTTP headers storage with case-insensitive keys.
/// @details Extends std::unordered_map<std::string, std::string>, but overrides
///          operator[] and find() to normalize keys via `utils::normalize`
///          (trim + lowercase). Values are stored as-is, only keys are normalized.
///          This matches RFC 7230 §3.2: header field names are case-insensitive.
struct headers_t final : public std::unordered_map<std::string, std::string>
{
  /* --------------- operator[] overriding zone --------------- */

  /// @brief   Returns reference to the value for `key` (inserting empty string if absent).
  /// @details `key` is normalized (trim + lowercase) before lookup.
  std::string &operator[](const std::string &key);
  /// @copydoc operator[](const std::string &)
  std::string &operator[](std::string &&key);

  /* ------------------ find overriding zone ------------------ */

  /// @brief  Finds element with normalized `key`.
  /// @return Iterator to element, or end() if not found.
  iterator find(const std::string &key);
  /// @copydoc find(const std::string &)
  iterator find(std::string &&key);
  /// @copydoc find(const std::string &)
  const_iterator find(const std::string &key) const;
  /// @copydoc find(const std::string &)
  const_iterator find(std::string &&key) const;

  /// @brief   Parses headers from an input stream up to the first empty line.
  /// @details Stops at CRLF CRLF boundary or EOF. Keys are normalized via
  ///          `utils::normalize`; values are trimmed. Lines without `:` are
  ///          silently skipped. Used by both request and response parsing.
  /// @return  Parsed headers (empty if the first line is already empty).
  [[nodiscard]] static headers_t parse(std::istream &stream);

  /// @brief  Parses the "Content-Length" header.
  /// @return Parsed length as std::size_t, or 0 if the header is absent.
  /// @throws core::http_error if the value is empty, non-numeric, or has trailing garbage.
  [[nodiscard]] std::size_t content_length() const;

  /// @brief  Computes how many body bytes still need to be read to satisfy Content-Length.
  /// @param  body Currently-read body (used as the "actual" size).
  /// @return 0 if body.size() >= expected; otherwise expected - body.size().
  [[nodiscard]] std::size_t remaining(const std::string &body) const;
};

/// @brief HTTP version string used in serialized messages.
constexpr inline std::string_view http_version = "HTTP/1.1";

} // namespace http
