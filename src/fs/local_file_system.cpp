#include "fs/local_file_system.hpp"

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <core/errors.hpp>

namespace fs
{

static const core::not_implemented_error err("func is not implemented yet");

void local_file_system::logging_error()
{
  [[maybe_unused]] const auto msg = m_error.message();
  // TODO: logging error message and error category
  m_error.clear();
}

std::vector<std::string> local_file_system::get_directory_files(const std::string_view &path)
{
  std::vector<std::string> directory_files;
  boost::filesystem::directory_iterator dir_iter { path };
  for (auto &&entry : dir_iter)
  {
    const auto &entry_path = entry.path();
    if (boost::filesystem::is_directory(entry_path))
    {
      auto subdirectory_files = get_directory_files(entry_path.string());
      directory_files.insert(directory_files.end(),            //
          std::make_move_iterator(subdirectory_files.begin()), //
          std::make_move_iterator(subdirectory_files.end()));  //
    }
    else
      directory_files.push_back(std::move(entry_path.string()));
  }
  return directory_files;
}

std::istream local_file_system::open(const std::string_view &path)
{
  boost::iostreams::mapped_file_params params { path };
  params.length = size(path);
  params.flags = boost::iostreams::mapped_file::mapmode::readonly;
  //  boost::iostreams::stream<boost::iostreams::mapped_file_sink> out { params };
  //  return std::istream { out.rdbuf() };
  boost::iostreams::stream_buffer<boost::iostreams::mapped_file_sink> buf { params };
  return std::istream { &buf };
}

std::ostream local_file_system::create(const std::string_view &path)
{
  boost::iostreams::stream_buffer<boost::iostreams::file_sink> buf(path.data());
  return std::ostream { &buf };
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

std::vector<std::string> local_file_system::list_files(const std::string_view &path)
{
  boost::filesystem::path well_path { path };
  if (is_directory(path))
    return get_directory_files(path);
  else
    return { std::string(path) };
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
