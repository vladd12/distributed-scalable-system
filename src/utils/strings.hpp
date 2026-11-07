#include <string>

namespace utils
{

/// @brief   Trim the passed const string.
/// @details Doesn't modify the passed string.
[[nodiscard]] std::string trim(const std::string &in);

/// @brief   Trim the passed non-const string.
/// @details Modifies the passed string.
std::string trim(std::string &&in);

/// @brief   Converts the passed const string to lower case.
/// @details Doesn't modify the passed string.
[[nodiscard]] std::string to_lower(const std::string &in);

/// @brief   Converts the passed non-const string to lower case.
/// @details Modifies the passed string.
std::string to_lower(std::string &&in);

/// @brief   Normalizes the passed const string: applies `trim` and `to_lower` functions.
/// @details Doesn't modify the passed string.
[[nodiscard]] std::string normalize(const std::string &in);

/// @brief   Normalizes the passed non-const string: applies `trim` and `to_lower` functions.
/// @details Modifies the passed string.
std::string normalize(std::string &&in);

} // namespace utils
