#pragma once

#include <boost/noncopyable.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace fs
{

struct file
{
  std::string name;
  std::uint64_t id;
};

struct file_filter
{
};

class file_system : private boost::noncopyable, public std::enable_shared_from_this<file_system>
{
public:
  typedef std::shared_ptr<file_system> ptr;

  virtual ~file_system()
  {
  }

  ptr getptr()
  {
    return shared_from_this();
  }

  virtual file open() = 0;
  virtual file create() = 0;
  virtual bool rename(file &f, const std::string_view &new_name) = 0;
  virtual bool remove(file &f) = 0;

  virtual bool is_exists(file &f) = 0;
  virtual bool is_directory(file &f) = 0;

  virtual std::uint64_t size(file &f) = 0;
  virtual std::vector<file> list_files(file_filter &filter) = 0;

  virtual void mkdir(file &f) = 0;

  virtual void lock(file &f, bool is_shared) = 0;
  virtual void release(file &f) = 0;

  virtual void copy_from_local(file &src, file &dst) = 0;
  virtual void move_from_local(file &src, file &dst) = 0;
  virtual void copy_to_local(file &src, file &dst) = 0;
  virtual void move_to_local(file &src, file &dst) = 0;

  /// \brief No more filesystem operations are needed.  Will release any held locks.
  virtual void close() = 0;
};

using file_system_ptr = file_system::ptr;

} // namespace fs
