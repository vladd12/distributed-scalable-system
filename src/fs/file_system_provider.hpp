#pragma once

#include <fs/file_system.hpp>
#include <unordered_map>

namespace fs
{

class file_system_provider
{
private:
  std::unordered_map<std::string, file_system_ptr> m_filesystems;

  void append(const std::string &host_formated, file_system_ptr &fs);

public:
  file_system_ptr get(const std::string &host, std::uint64_t port);
};

} // namespace fs
