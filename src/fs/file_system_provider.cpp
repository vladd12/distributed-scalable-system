#include "fs/file_system_provider.hpp"

#include <fmt/core.h>
#include <fs/distributed_file_system.hpp>
#include <fs/local_file_system.hpp>

namespace fs
{

file_system_provider::file_system_provider(config::configuration_ptr cfg)
  : m_cfg(cfg)
{
}

void file_system_provider::append(const std::string &host_formated, file_system_ptr &fs)
{
  auto val = m_filesystems.insert({ host_formated, fs });
}

file_system_ptr file_system_provider::get(const std::string &host, std::uint64_t port)
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
      file_system_ptr fs { std::move(std::make_shared<local_file_system>()) };
      append(host_formated, fs);
      return fs;
    }
    else
    {
      file_system_ptr fs { std::move(std::make_shared<distributed_file_system>(m_cfg->dfs)) };
      append(host_formated, fs);
      return fs;
    }
  }
}

} // namespace fs
