#include "fs/distributed_file_system.hpp"

#include <core/errors.hpp>

namespace fs
{

static const core::not_implemented_error err("func is not implemented yet");

file distributed_file_system::open()
{
  throw err;
}

file distributed_file_system::create()
{
  throw err;
}

bool distributed_file_system::rename(file &f, const std::string_view &new_name)
{
  throw err;
}

bool distributed_file_system::remove(file &f)
{
  throw err;
}

bool distributed_file_system::is_exists(file &f)
{
  throw err;
}

bool distributed_file_system::is_directory(file &f)
{
  throw err;
}

std::uint64_t distributed_file_system::size(file &f)
{
  throw err;
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
