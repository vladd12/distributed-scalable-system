#pragma once

#include <boost/noncopyable.hpp>
#include <fmt/core.h>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

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
  virtual ~file_system()
  {
  }

  std::shared_ptr<file_system> getptr()
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

using file_system_ptr = std::shared_ptr<file_system>;

class local_file_system : public file_system
{
public:
};

class distributed_file_system : public file_system
{
public:
};

class file_system_provider
{
private:
  std::unordered_map<std::string, file_system_ptr> m_filesystems;

  void append(const std::string &host_formated, file_system_ptr &fs)
  {
    auto val = m_filesystems.insert({ host_formated, fs });
  }

public:
  file_system_ptr get(const std::string &host, std::uint64_t port)
  {
    auto host_formated { std::move(fmt::format("{0}:{1}", host, port)) };
    if (auto search = m_filesystems.find(host_formated); search != m_filesystems.end())
    {
      return search->second;
    }
    else
    {
      if (host == "localhost")
      {
        file_system_ptr fs { std::make_shared<local_file_system>() };
        append(host_formated, fs);
        return fs;
      }
      else
      {
        file_system_ptr fs { std::make_shared<distributed_file_system>() };
        append(host_formated, fs);
        return fs;
      }
    }
  }
};

} // namespace fs
