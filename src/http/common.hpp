#pragma once

#include <iosfwd> // std::istream
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

/// \brief Type for representation headers in HTTP request/response.
using headers_t = std::unordered_map<std::string, std::string>;

/// \brief Parsing HTTP headers from the incoming input stream.
headers_t parse_headers(std::istream &stream);

/// \brief   Compares the expected content length from headers with the actual size of bofy for HTTP requests/responses.
/// \details Used as common part in HTTP requests/responses.
std::size_t get_remaining_data_length(const headers_t &headers, const std::string &body) noexcept;

constexpr inline std::string_view http_version = "HTTP/1.1";

} // namespace http
