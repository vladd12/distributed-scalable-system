#pragma once

#include <boost/filesystem.hpp>
#include <string_view>

namespace fs
{

class file
{
private:
  boost::filesystem::path m_path;
  boost::system::error_code m_err;

public:
  file(const boost::filesystem::path &path) : m_path(path)
  {
  }

  file(const std::string_view &path) : m_path(path)
  {
  }

  file(const file &parent, const std::string_view &child) : m_path(parent.m_path / child)
  {
  }

  file(const std::string_view &parent, const std::string_view &child) : file(file(parent), child)
  {
  }

  bool is_file() noexcept
  {
    return !(is_directory());
  }

  bool is_directory() noexcept
  {
    return boost::filesystem::is_directory(m_path, m_err);
  }

  file get_parent()
  {
    return m_path.parent_path();
  }

  std::string get_path()
  {
    return m_path.string();
  }

  //  bool mkdir()
  //  {
  //  }
};

} // namespace fs
