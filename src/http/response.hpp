#pragma once

#include <http/common.hpp>

namespace http
{

/// \brief Represents a parsed status line from HTTP response.
struct status_line
{
  unsigned int status_code = 200;
  std::string version;

  static status_line parse(std::istream &stream);
};

/// \brief Represents an HTTP response to be sent back to the client.
struct response
{
  status_line line;
  // unsigned int status_code = 200;
  // std::string version;
  headers_t headers;
  std::string body;

  /// \brief Serializes the full HTTP response into a string.
  std::string serialize() const;

  /// \brief   Returns size of remaining data for reading from incoming HTTP response.
  /// \details Used in HTTP client.
  std::size_t remaining() const noexcept;

  /// \brief Creates a plain text response.
  static response text(unsigned int code, const std::string_view &text);

  /// \brief Creates a JSON response.
  static response json(unsigned int code, const std::string_view &json_body);

  /// \brief Parses a raw HTTP response from an input stream.
  static response parse(std::istream &stream);

  /// \brief Parses a raw HTTP response from a string (for testing or simple usage).
  static response parse(const std::string &data);
};

} // namespace http
