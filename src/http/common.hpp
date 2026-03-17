#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace http
{

/// \brief Supported HTTP request methods.
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

/// \brief Converts a string to an HTTP method.
method string_to_method(const std::string_view str);

/// \brief Converts an HTTP method to its string representation.
std::string_view method_to_string(const method m);

/// \brief Returns a standard reason phrase for the given HTTP status code.
std::string_view status_text_for(const unsigned int code);

using headers_t = std::unordered_map<std::string, std::string>;

} // namespace http
