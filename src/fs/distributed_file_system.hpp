#pragma once

#include <fs/file_system.hpp>

namespace fs
{

class distributed_file_system final : public file_system
{
public:
  std::istream open(const std::string_view &path) override;
  std::ostream create(const std::string_view &path) override;

  bool rename(const std::string_view &old_path, const std::string_view &new_path) override;
  bool remove(const std::string_view &path) override;

  bool is_exists(const std::string_view &path) noexcept override;
  bool is_directory(const std::string_view &path) noexcept override;
  std::uint64_t size(const std::string_view &path) noexcept override;

  std::vector<std::string> list_files(const std::string_view &path) override;

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
