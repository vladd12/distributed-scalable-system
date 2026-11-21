#include "http/headers.hpp"

#include <core/errors.hpp>
#include <utils/strings.hpp>
//
#include <array>
#include <charconv>
#include <istream>

namespace
{
/// @brief   Headers that must appear at most once per HTTP message.
/// @details Duplicates of these are rejected: `Content-Length` duplication is
///          a request-smuggling vector (RFC 7230 §3.3.2), `Host` duplication is
///          an invalid HTTP/1.1 request (RFC 7230 §5.4). Keys are expected in
///          normalized (lowercase) form.
constexpr inline std::array<std::string_view, 2> forbidden_duplicate_headers {
  "content-length",
  "host",
};

[[nodiscard]] inline bool is_duplicate_forbidden(std::string_view normalized_key) noexcept
{
  for (auto key : forbidden_duplicate_headers)
  {
    if (normalized_key == key)
      return true;
  }
  return false;
}

} // anonymous namespace

namespace http
{

headers_t::headers_t(std::initializer_list<value_type> init_list)
{
  for (auto &&[key, value] : init_list)
    storage.insert({ utils::normalize(key), utils::trim(value) });
}

std::string &headers_t::operator[](const std::string &key)
{
  return storage[utils::normalize(key)];
}

std::string &headers_t::operator[](std::string &&key)
{
  return storage[utils::normalize(std::move(key))];
}

headers_t::iterator headers_t::find(const std::string &key)
{
  return storage.find(utils::normalize(key));
}

headers_t::const_iterator headers_t::find(const std::string &key) const
{
  return storage.find(utils::normalize(key));
}

headers_t headers_t::parse(std::istream &stream)
{
  headers_t headers;
  std::string line;

  // Headers parsing
  while (std::getline(stream, line))
  {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    if (line.empty())
      break;

    auto colon = line.find(':');
    if (colon == 0 || colon == std::string::npos)
      throw core::http_error("headers parsing error: invalid header passed");

    std::string key = utils::normalize(line.substr(0, colon));
    std::string value = utils::trim(line.substr(colon + 1));
    if (is_duplicate_forbidden(key) && headers.storage.contains(key))
      throw core::http_error("headers parsing error: duplicate '" + key + "' header");

    headers.storage.insert({ std::move(key), utils::trim(value) });
  }

  return headers;
}

std::size_t headers_t::content_length() const
{
  const auto find_iter = find("Content-Length");
  if (find_iter != cend())
  {
    const auto &value = find_iter->second;
    if (value.empty())
      throw core::http_error("headers parsing error: invalid 'Content-Length' header");

    std::size_t content_length = 0;
    const char *begin = value.data();
    const char *end = value.data() + value.size();
    const auto [parse_end, parse_error] = std::from_chars(begin, end, content_length);
    if (parse_error != std::errc() || parse_end != end)
      throw core::http_error("headers parsing error: invalid 'Content-Length' header");

    return content_length;
  }
  else
    return 0;
}

std::size_t headers_t::remaining(const std::string &body) const
{
  const std::size_t expected = content_length();
  const std::size_t actual = body.length();
  if (actual < expected)
    return expected - actual;
  else
    return 0;
}

} // namespace http
