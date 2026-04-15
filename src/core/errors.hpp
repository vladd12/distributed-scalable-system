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

/// \brief Class for file system errors (file not found).
class file_not_found final : public io_error
{
public:
  inline explicit file_not_found(const std::string &msg) : io_error(msg)
  {
  }

  inline explicit file_not_found(const char *msg) : io_error(msg)
  {
  }
};

/// \brief Class for HTTP errors (connection errors, etc...).
class http_error final : public std::runtime_error
{
public:
  inline explicit http_error(const std::string &msg) : std::runtime_error(msg)
  {
  }

  inline explicit http_error(const char *msg) : std::runtime_error(msg)
  {
  }
};

} // namespace core
