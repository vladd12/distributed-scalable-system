#pragma once

#include <stdexcept>

namespace core
{

/// \brief Class for not implemented features.
class not_implemented_error final : public std::runtime_error
{
public:
  inline explicit not_implemented_error(const std::string &msg) : std::runtime_error(msg)
  {
  }

  inline explicit not_implemented_error(const char *msg) : std::runtime_error(msg)
  {
  }
};

/// \brief Class for IO errors.
class io_error : public std::runtime_error
{
public:
  inline explicit io_error(const std::string &msg) : std::runtime_error(msg)
  {
  }

  inline explicit io_error(const char *msg) : std::runtime_error(msg)
  {
  }
};

/// \brief Class for checksum errors.
class checksum_error final : public io_error
{
public:
  inline explicit checksum_error(const std::string &msg) : io_error(msg)
  {
  }

  inline explicit checksum_error(const char *msg) : io_error(msg)
  {
  }
};

} // namespace core
