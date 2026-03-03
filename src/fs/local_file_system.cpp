#include "fs/local_file_system.hpp"

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <core/errors.hpp>
#include <fmt/core.h>

namespace
{

void logging_error(const boost::system::error_code &error)
{

  [[maybe_unused]] const auto msg = error.message();
  // TODO: logging error message and error category
}

} // anonymous namespace

namespace fs
{

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

std::size_t local_file_system::get_hash_for(const std::string_view &path)
{
  std::hash<std::string_view> str_hasher;
  return str_hasher(path);
}

std::string local_file_system::get_lock_file_path_for(const std::size_t path_hash)
{
  boost::system::error_code error;
  auto temp_path = boost::filesystem::temp_directory_path(error);
  if (error)
  {
    ::logging_error(error);
    throw core::io_error("Can't detect path for temporary files");
  }
  const auto temp_path_str = temp_path.string();
  return fmt::vformat("{}/dcs/{}.lock", fmt::make_format_args(temp_path_str, path_hash));
}

std::istream local_file_system::open(const std::string_view &path)
{
  if (!is_exists(path))
    throw core::file_not_found(fmt::format("File not found: {}", path));

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
  boost::system::error_code error;
  boost::filesystem::rename(old_path, new_path, error);
  if (error)
  {
    ::logging_error(error);
    return false;
  }
  else
    return true;
}

bool local_file_system::remove(const std::string_view &path)
{
  boost::system::error_code error;
  auto result = boost::filesystem::remove(path, error);
  if (error)
  {
    ::logging_error(error);
    return false;
  }
  else
    return result;
}

bool local_file_system::copy(const std::string_view &src, const std::string_view &dst)
{
  boost::system::error_code error;
  boost::filesystem::copy(src, dst, error);
  if (error)
  {
    ::logging_error(error);
    return false;
  }
  else
    return true;
}

bool local_file_system::move(const std::string_view &src, const std::string_view &dst)
{
  return rename(src, dst);
}

bool local_file_system::is_exists(const std::string_view &path) const noexcept
{
  return boost::filesystem::exists(path);
}

bool local_file_system::is_directory(const std::string_view &path) const noexcept
{
  return boost::filesystem::is_directory(path);
}

std::uint64_t local_file_system::size(const std::string_view &path) const noexcept
{
  boost::system::error_code error;
  auto size = boost::filesystem::file_size(path, error);
  if (error)
  {
    ::logging_error(error);
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

bool local_file_system::mkdir(const std::string_view &path) noexcept
{
  boost::system::error_code error;
  auto result = boost::filesystem::create_directory(path, error);
  if (error)
  {
    ::logging_error(error);
    return false;
  }
  else
    return result;
}

bool local_file_system::copy_from_local(const std::string_view &src, const std::string_view &dst)
{
  return copy(src, dst);
}

bool local_file_system::move_from_local(const std::string_view &src, const std::string_view &dst)
{
  return move(src, dst);
}

bool local_file_system::copy_to_local(const std::string_view &src, const std::string_view &dst)
{
  return copy(src, dst);
}

bool local_file_system::move_to_local(const std::string_view &src, const std::string_view &dst)
{
  return move(src, dst);
}

void local_file_system::lock(const std::string_view &path, bool is_shared)
{
  const auto path_hash = get_hash_for(path);
  const auto lock_file_path = get_lock_file_path_for(path_hash);
  auto lock_file_ostream = create(lock_file_path);
  lock_file_ostream << path_hash << '\n';
  lock_file_ostream.flush();
}

void local_file_system::release(const std::string_view &path)
{
  const auto path_hash = get_hash_for(path);
  const auto lock_file_path = get_lock_file_path_for(path_hash);
  if (!is_exists(lock_file_path))
    throw core::io_error("Given target not held as lock");

  if (!remove(lock_file_path))
    throw core::io_error(fmt::format("Can't remove lock file: {}", lock_file_path));
}

void local_file_system::close()
{
  // just do nothing
}

} // namespace fs
