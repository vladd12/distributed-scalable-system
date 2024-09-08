#include "fs/distributed_file_system.hpp"

#include <core/errors.hpp>

namespace fs
{

static const core::not_implemented_error err("func is not implemented yet");

std::istream distributed_file_system::open(const std::string_view &path)
{
  throw err;
}

std::ostream distributed_file_system::create(const std::string_view &path)
{
  throw err;
}

bool distributed_file_system::rename(const std::string_view &old_path, const std::string_view &new_path)
{
  throw err;
}

bool distributed_file_system::remove(const std::string_view &path)
{
  throw err;
}

bool distributed_file_system::is_exists(const std::string_view &path) noexcept
{
  /// TODO: implement this
  return false;
}

bool distributed_file_system::is_directory(const std::string_view &path) noexcept
{
  /// TODO: implement this
  return false;
}

std::uint64_t distributed_file_system::size(const std::string_view &path) noexcept
{
  /// TODO: implement this
  return 0;
}

std::vector<file> distributed_file_system::list_files(file_filter &filter)
{
  throw err;
}

void distributed_file_system::mkdir(file &f)
{
  throw err;
}

void distributed_file_system::lock(file &f, bool is_shared)
{
  throw err;
}

void distributed_file_system::release(file &f)
{
  throw err;
}

void distributed_file_system::copy_from_local(file &src, file &dst)
{
  throw err;
}

void distributed_file_system::move_from_local(file &src, file &dst)
{
}

void distributed_file_system::copy_to_local(file &src, file &dst)
{
  throw err;
}

void distributed_file_system::move_to_local(file &src, file &dst)
{
  throw err;
}

void distributed_file_system::close()
{
  // just do nothing
}

} // namespace fs
