#pragma once

#include <fs/file_system.hpp>

namespace fs
{

class distributed_file_system final : public file_system
{
public:
  file open() override;
  file create() override;
  bool rename(file &f, const std::string_view &new_name) override;
  bool remove(file &f) override;

  bool is_exists(file &f) override;
  bool is_directory(file &f) override;

  std::uint64_t size(file &f) override;
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
