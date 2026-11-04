#include "utils/strings.hpp"

#include <algorithm>
#include <cctype>

namespace
{
constexpr inline std::string_view whitespaces = " \t\n\r\f\v";
}

namespace utils
{

std::string trim(const std::string &in)
{
  using size_t = std::string::size_type;
  size_t start = in.find_first_not_of(whitespaces);
  // If string contains only whitespaces
  if (start == std::string::npos)
    return "";

  size_t end = in.find_last_not_of(whitespaces);
  return in.substr(start, end - start + 1);
}

std::string &trim(std::string &in)
{
  using size_t = std::string::size_type;
  size_t start = in.find_first_not_of(whitespaces);
  // If string contains only whitespaces
  if (start == std::string::npos)
    start = in.size();
  // Trim from the start
  if (start > 0)
    in.erase(0, start);
  // Trim from the end
  size_t end = in.find_last_not_of(whitespaces);
  if (end != std::string::npos)
    in.erase(end + 1);

  return in;
}

std::string to_lower(const std::string &in)
{
  std::string result(in.begin(), in.end());
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string &to_lower(std::string &in)
{
  std::transform(in.begin(), in.end(), in.begin(), [](unsigned char c) { return std::tolower(c); });
  return in;
}

std::string normalize(const std::string &in)
{
  return to_lower(trim(in));
}

std::string &normalize(std::string &in)
{
  return to_lower(trim(in));
}

} // namespace utils
