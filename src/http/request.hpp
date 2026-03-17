#pragma once

#include <http/common.hpp>

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
};

} // namespace http
