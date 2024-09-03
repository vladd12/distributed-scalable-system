#pragma once

#include <boost/system/error_code.hpp>
#include <fs/file_system.hpp>
#include <iostream>

namespace fs
{

class local_file_system final : public file_system
{
private:
  boost::system::error_code m_error;

  void logging_error();

public:
  file open() override;
  file create() override;
  bool rename(file &f, const std::string_view &new_name) override;
  bool remove(file &f) override;

  bool is_exists(file &f) override;
  bool is_directory(file &f) override;
  std::uint64_t size(file &f) override;

  std::ostream open(const std::string_view &path);
  void create(const std::string_view &path);

  bool rename(const std::string_view &old_path, const std::string_view &new_path);
  bool remove(const std::string_view &path);

  bool is_exists(const std::string_view &path) noexcept;
  bool is_directory(const std::string_view &path) noexcept;
  std::uint64_t size(const std::string_view &path) noexcept;

  std::vector<file> list_files(file_filter &filter) override;

  void mkdir(file &f) override;

  void lock(file &f, bool is_shared) override;
  void release(file &f) override;

  void copy_from_local(file &src, file &dst) override;
  void move_from_local(file &src, file &dst) override;
  void copy_to_local(file &src, file &dst) override;
  void move_to_local(file &src, file &dst) override;

  void close() override;
};

} // namespace fs
