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

  explicit file_system() noexcept = default;
  virtual ~file_system() noexcept = default;

  ptr get_ptr()
  {
    return shared_from_this();
  }

  /// \brief Opens a file for reading
  /// \param path The file path to open
  virtual std::istream open(const std::string_view &path) = 0;

  /// \brief Creates a file for writing
  /// \param path The file path to create
  virtual std::ostream create(const std::string_view &path) = 0;

  virtual bool rename(const std::string_view &old_path, const std::string_view &new_path) = 0;
  virtual bool remove(const std::string_view &path) = 0;
  virtual bool copy(const std::string_view &src, const std::string_view &dst) = 0;
  virtual bool move(const std::string_view &src, const std::string_view &dst) = 0;

  /// \brief Checks if a file or directory exists
  /// \param path The path to check
  /// \return true if exists, false otherwise
  virtual bool is_exists(const std::string_view &path) const noexcept = 0;

  /// \brief Checks if the given path is a directory
  /// \param path The path to check
  /// \return true if directory, false otherwise
  virtual bool is_directory(const std::string_view &path) const noexcept = 0;

  /// \brief Gets the size of a file
  /// \param path The file path
  /// \return Size in bytes, 0 if file doesn't exist or is directory
  virtual std::uint64_t size(const std::string_view &path) const noexcept = 0;

  /// \brief Lists all files in the given directory
  /// \param path The directory path to list
  /// \return Vector of file/directory names, empty if path doesn't exist or is not a directory
  /// \throws core::io_error if there's an I/O error accessing the directory
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

  virtual bool copy_from_local(const std::string_view &src, const std::string_view &dst) = 0;
  virtual bool move_from_local(const std::string_view &src, const std::string_view &dst) = 0;
  virtual bool copy_to_local(const std::string_view &src, const std::string_view &dst) = 0;
  virtual bool move_to_local(const std::string_view &src, const std::string_view &dst) = 0;

  virtual void lock(const std::string_view &path, bool is_shared) = 0;
  virtual void release(const std::string_view &path) = 0;

  /// \brief No more filesystem operations are needed.  Will release any held locks.
  virtual void close() = 0;
};

using file_system_ptr = file_system::ptr;

} // namespace fs
