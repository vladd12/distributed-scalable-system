#pragma once

#include <fs/file_system.hpp>

namespace fs
{

class local_file_system final : public file_system
{
private:
  std::vector<std::string> get_directory_files(const std::string_view &path);
  std::size_t get_hash_for(const std::string_view &path);
  std::string get_lock_file_path_for(const size_t hash);

public:
  std::istream open(const std::string_view &path) override;
  std::ostream create(const std::string_view &path) override;

  bool rename(const std::string_view &old_path, const std::string_view &new_path) override;
  bool remove(const std::string_view &path) override;
  bool copy(const std::string_view &src, const std::string_view &dst) override;
  bool move(const std::string_view &src, const std::string_view &dst) override;

  bool is_exists(const std::string_view &path) const noexcept override;
  bool is_directory(const std::string_view &path) const noexcept override;
  std::uint64_t size(const std::string_view &path) const noexcept override;

  std::vector<std::string> list_files(const std::string_view &path) override;

  bool mkdir(const std::string_view &path) noexcept override;

  bool copy_from_local(const std::string_view &src, const std::string_view &dst) override;
  bool move_from_local(const std::string_view &src, const std::string_view &dst) override;
  bool copy_to_local(const std::string_view &src, const std::string_view &dst) override;
  bool move_to_local(const std::string_view &src, const std::string_view &dst) override;

  void lock(const std::string_view &path, bool is_shared) override;
  void release(const std::string_view &path) override;

  void close() override;
};

} // namespace fs
