#pragma once

#include <http/common.hpp>

namespace http
{

/// \brief Represents a parsed request line from HTTP request.
struct request_line
{
  method method_type = method::UNKNOWN;
  std::string path;
  std::string version;

  [[nodiscard]] static request_line parse(std::istream &stream);
};

/// \brief Represents a parsed HTTP request.
struct request
{
  request_line line;
  headers_t headers;
  std::string body;

  /// \brief Serializes the full HTTP request into a string.
  [[nodiscard]] std::string serialize() const;

  /// \brief Returns size of remaining data for reading from incoming HTTP request.
  /// \details Used in HTTP server.
  [[nodiscard]] std::size_t remaining() const noexcept;

  /// \brief Parses a raw HTTP request from an input stream.
  [[nodiscard]] static request parse(std::istream &stream);

  /// \brief Parses a raw HTTP request from a string (for testing or simple usage).
  [[nodiscard]] static request parse(const std::string &data);
};

} // namespace http
