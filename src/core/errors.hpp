#pragma once

#include <stdexcept>

namespace core
{

/// \brief Class for not implemented features.
class not_implemented_error final : public std::runtime_error
{
public:
  explicit not_implemented_error(const std::string &msg) : std::runtime_error(msg)
  {
  }

  explicit not_implemented_error(const char *msg) : std::runtime_error(msg)
  {
  }
};

} // namespace core
