#pragma once

#include <boost/noncopyable.hpp>
#include <iostream>
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
public:
  virtual bool operator()(const std::string_view &path) const = 0;
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

  virtual std::istream open(const std::string_view &path) = 0;
  virtual std::ostream create(const std::string_view &path) = 0;

  virtual bool rename(const std::string_view &old_path, const std::string_view &new_path) = 0;
  virtual bool remove(const std::string_view &path) = 0;

  virtual bool is_exists(const std::string_view &path) noexcept = 0;
  virtual bool is_directory(const std::string_view &path) noexcept = 0;
  virtual std::uint64_t size(const std::string_view &path) noexcept = 0;

  virtual std::vector<std::string> list_files(const std::string_view &path) = 0;

  template <typename F> //
  inline std::vector<std::string> list_files(const std::string_view &path, F &&filter)
  {
    auto all_files = list_files(path);
    std::vector<std::string> filtered_files;
    for (auto &&file : all_files)
    {
      if (filter(file))
        filtered_files.push_back(file);
    }
    return filtered_files;
  }

  std::vector<std::string> list_files(const std::string_view &path, file_filter &filter)
  {
    return list_files<file_filter>(path, std::move(filter));
  }

  virtual bool mkdir(const std::string_view &path) noexcept = 0;

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
