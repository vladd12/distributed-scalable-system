#include "fs/local_file_system.hpp"

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <core/errors.hpp>

namespace fs
{

static const core::not_implemented_error err("func is not implemented yet");

file local_file_system::open()
{
  throw err;
}

file local_file_system::create()
{
  throw err;
}

bool local_file_system::rename(file &f, const std::string_view &new_name)
{
  throw err;
}

bool local_file_system::remove(file &f)
{
  throw err;
}

bool local_file_system::is_exists(file &f)
{
  throw err;
}

bool local_file_system::is_directory(file &f)
{
  throw err;
}

std::uint64_t local_file_system::size(file &f)
{
  throw err;
}

void local_file_system::logging_error()
{
  [[maybe_unused]] const auto msg = m_error.message();
  // TODO: logging error message and error category
  m_error.clear();
}

std::ostream local_file_system::open(const std::string_view &path)
{
  boost::iostreams::mapped_file_params params { path };
  params.length = size(path);
  params.flags = boost::iostreams::mapped_file::mapmode::readonly;
  boost::iostreams::stream<boost::iostreams::mapped_file_sink> out { params };
  return std::ostream { out.rdbuf() };
}

void local_file_system::create(const std::string_view &path)
{
  ;
}

bool local_file_system::rename(const std::string_view &old_path, const std::string_view &new_path)
{
  boost::filesystem::rename(old_path, new_path, m_error);
  if (m_error)
  {
    logging_error();
    return false;
  }
  else
    return true;
}

bool local_file_system::remove(const std::string_view &path)
{
  auto result = boost::filesystem::remove(path, m_error);
  if (m_error)
  {
    logging_error();
    return false;
  }
  else
    return result;
}

bool local_file_system::is_exists(const std::string_view &path) noexcept
{
  return boost::filesystem::exists(path);
}

bool local_file_system::is_directory(const std::string_view &path) noexcept
{
  return boost::filesystem::is_directory(path);
}

std::uint64_t local_file_system::size(const std::string_view &path) noexcept
{
  auto size = boost::filesystem::file_size(path, m_error);
  if (m_error)
  {
    logging_error();
    return 0;
  }
  else
    return size;
}

std::vector<file> local_file_system::list_files(file_filter &filter)
{
  throw err;
}

void local_file_system::mkdir(file &f)
{
  throw err;
}

void local_file_system::lock(file &f, bool is_shared)
{
  throw err;
}

void local_file_system::release(file &f)
{
  throw err;
}

void local_file_system::copy_from_local(file &src, file &dst)
{
  throw err;
}

void local_file_system::move_from_local(file &src, file &dst)
{
}

void local_file_system::copy_to_local(file &src, file &dst)
{
  throw err;
}

void local_file_system::move_to_local(file &src, file &dst)
{
  throw err;
}

void local_file_system::close()
{
  // just do nothing
}

} // namespace fs
