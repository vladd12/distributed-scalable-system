#pragma once

#include <http/common.hpp>
#include <iosfwd> // std::istringstream

namespace http
{

/// \brief Represents a parsed HTTP request.
struct request
{
  method method_type = method::UNKNOWN;
  std::string path;
  std::string version;
  headers_t headers;
  std::string body;

  /// \brief Serializes the full HTTP request into a string.
  std::string serialize() const;

  /// \brief Parses a raw HTTP request from an input stream.
  static request parse(std::istream &stream);

  /// \brief Parses a raw HTTP request from a string (for testing or simple usage).
  static request parse(const std::string &data);
};

} // namespace http
