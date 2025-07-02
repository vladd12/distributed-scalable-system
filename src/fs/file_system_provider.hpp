#pragma once

#include <config/configuration_v1.hpp>
#include <fs/file_system.hpp>
#include <unordered_map>

namespace fs
{

class file_system_provider
{
private:
  std::unordered_map<std::string, file_system_ptr> m_filesystems;
  config::configuration_ptr m_cfg;

  void append(const std::string &host_formated, file_system_ptr &fs);

public:
  explicit file_system_provider(config::configuration_ptr cfg);
  file_system_ptr get(const std::string &host, std::uint64_t port);
};

} // namespace fs
