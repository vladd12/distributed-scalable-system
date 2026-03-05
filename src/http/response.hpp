#pragma once

#include <http/common.hpp>
#include <string>
#include <unordered_map>

namespace http
{

/// \brief Represents an HTTP response to be sent back to the client.
struct response
{
  unsigned int status_code = 200;
  std::unordered_map<std::string, std::string> headers;
  std::string body;

  /// \brief Serializes the full HTTP response into a string.
  std::string serialize() const;

  /// \brief Creates a plain text response.
  static response text(unsigned int code, std::string_view text);

  /// \brief Creates a JSON response.
  static response json(unsigned int code, std::string_view json_body);
};

} // namespace http
