#pragma once

#include <http/common.hpp>
#include <string>
#include <unordered_map>

namespace http
{

/// \brief Represents a parsed HTTP request.
struct request
{
  method method_type = method::UNKNOWN;
  std::string path;
  std::string version;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
};

} // namespace http
