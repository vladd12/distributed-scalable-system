#include "fs/distributed_file_system.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <core/errors.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs
{

static const core::not_implemented_error err("func is not implemented yet");

// Distributed file system implementation
distributed_file_system::distributed_file_system(const config::name_node_configuration &name_node_config)
    : m_name_node(std::make_unique<dfs::name_node>(name_node_config.replication)), m_name_node_config(name_node_config)
{
  // Validate configuration
  if (name_node_config.data_nodes.empty())
  {
    throw std::invalid_argument("No data nodes configured. A distributed file system requires at least one data node.");
  }

  if (name_node_config.replication > name_node_config.data_nodes.size())
  {
    throw std::invalid_argument("Replication factor cannot be greater than the number of data nodes.");
  }

  // Initialize data nodes from configuration
  for (const auto &node_config : name_node_config.data_nodes)
  {
    // TODO: capacity and node name must be requested
    // node.node_name = node_config.node_name;
    // node.capacity = node_config.capacity;
    dfs::data_node node;
    node.host = node_config.host;
    node.port = node_config.port;
    node.used_space = 0;
    node.is_alive = true;
    node.last_heartbeat = std::chrono::system_clock::now();

    m_name_node->register_data_node(node);
  }
}

std::string distributed_file_system::generate_block_id()
{
  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  return boost::uuids::to_string(uuid);
}

bool distributed_file_system::transfer_data_to_nodes(
    const std::vector<dfs::data_node> &nodes, const std::string &block_id, const std::vector<char> &data)
{
  // In a real implementation, this would use network communication to transfer data to nodes
  // For now, simulate successful transfer
  return !nodes.empty();
}

std::vector<char> distributed_file_system::read_data_from_nodes(const dfs::file_block &block)
{
  // In a real implementation, this would read data from the data nodes
  // For now, return empty data
  return std::vector<char>();
}

bool distributed_file_system::replicate_block(
    const std::string &block_id, const std::vector<dfs::data_node> &target_nodes)
{
  // In a real implementation, this would handle block replication
  return true;
}

std::istream distributed_file_system::open(const std::string_view &path)
{
  // Open a file for reading from the distributed file system
  const auto metadata = m_name_node->get_metadata(std::string(path));
  if (!metadata || metadata->is_directory)
  {
    throw core::file_not_found("File not found or is a directory: " + std::string(path));
  }

  // For now, return a null istream - in a real implementation,
  // this would combine data from all blocks across data nodes
  return std::istream(nullptr);
}

std::ostream distributed_file_system::create(const std::string_view &path)
{
  // Create a new file in the distributed file system
  if (!m_name_node->create_file(std::string(path), false))
  {
    throw core::io_error("Failed to create file: " + std::string(path));
  }

  // For now, return a null ostream - in a real implementation,
  // this would write data to selected data nodes
  return std::ostream(nullptr);
}

bool distributed_file_system::rename(const std::string_view &old_path, const std::string_view &new_path)
{
  // Rename a file in the distributed file system
  return m_name_node->rename_file(std::string(old_path), std::string(new_path));
}

bool distributed_file_system::remove(const std::string_view &path)
{
  // Remove a file from the distributed file system
  return m_name_node->delete_file(std::string(path));
}

bool distributed_file_system::copy(const std::string_view &src, const std::string_view &dst)
{
  // Copy a file within the distributed file system
  const auto src_metadata = m_name_node->get_metadata(std::string(src));
  if (!src_metadata)
  {
    return false; // Source file not found
  }

  // Create destination file and copy metadata
  if (!m_name_node->create_file(std::string(dst), src_metadata->is_directory))
  {
    return false; // Failed to create destination
  }

  // In a real implementation, this would copy blocks between data nodes
  return true;
}

bool distributed_file_system::move(const std::string_view &src, const std::string_view &dst)
{
  // Move a file within the distributed file system
  return m_name_node->rename_file(std::string(src), std::string(dst));
}

bool distributed_file_system::is_exists(const std::string_view &path) const noexcept
{
  try
  {
    const auto metadata = m_name_node->get_metadata(std::string(path));
    return metadata != nullptr;
  } catch (...)
  {
    return false;
  }
}

bool distributed_file_system::is_directory(const std::string_view &path) const noexcept
{
  try
  {
    const auto metadata = m_name_node->get_metadata(std::string(path));
    return metadata && metadata->is_directory;
  } catch (...)
  {
    return false;
  }
}

std::uint64_t distributed_file_system::size(const std::string_view &path) const noexcept
{
  try
  {
    const auto metadata = m_name_node->get_metadata(std::string(path));
    return metadata ? metadata->size : 0;
  } catch (...)
  {
    return 0;
  }
}

std::vector<std::string> distributed_file_system::list_files(const std::string_view &path)
{
  return m_name_node->list_directory(std::string(path));
}

bool distributed_file_system::mkdir(const std::string_view &path) noexcept
{
  try
  {
    return m_name_node->create_file(std::string(path), true);
  } catch (...)
  {
    return false;
  }
}

void distributed_file_system::lock(const std::string_view &path, bool is_shared)
{
  std::lock_guard<std::mutex> lock(m_locks_mutex);

  const std::string path_str(path);
  if (m_path_locks.find(path_str) == m_path_locks.end())
  {
    m_path_locks[path_str] = std::make_unique<std::mutex>();
  }

  // In a real implementation, this would handle shared vs exclusive locks
  // For now, just acquire the mutex
  m_path_locks[path_str]->lock();
}

void distributed_file_system::release(const std::string_view &path)
{
  std::lock_guard<std::mutex> lock(m_locks_mutex);

  const std::string path_str(path);
  auto it = m_path_locks.find(path_str);
  if (it != m_path_locks.end())
  {
    it->second->unlock();
  }
}

bool distributed_file_system::copy_from_local(const std::string_view &src, const std::string_view &dst)
{
  try
  {
    // Copy from local file system to distributed file system
    if (!std::filesystem::exists(std::string(src)))
    {
      return false; // Source file doesn't exist locally
    }

    // Create file in DFS
    if (!m_name_node->create_file(std::string(dst), false))
    {
      return false; // Failed to create file in DFS
    }

    // In a real implementation, this would:
    // 1. Read the local file in blocks
    // 2. Generate block IDs
    // 3. Select data nodes for each block
    // 4. Transfer blocks to selected data nodes
    // 5. Update metadata in name node

    return true;
  } catch (...)
  {
    return false;
  }
}

bool distributed_file_system::move_from_local(const std::string_view &src, const std::string_view &dst)
{
  // Move = Copy + Delete local file
  if (copy_from_local(src, dst))
  {
    try
    {
      std::filesystem::remove(std::string(src));
      return true;
    } catch (...)
    {
      // Copy succeeded but delete failed
      return false;
    }
  }
  return false;
}

bool distributed_file_system::copy_to_local(const std::string_view &src, const std::string_view &dst)
{
  try
  {
    // Copy from distributed file system to local file system
    const auto metadata = m_name_node->get_metadata(std::string(src));
    if (!metadata || metadata->is_directory)
    {
      return false; // Source file not found or is a directory
    }

    // In a real implementation, this would:
    // 1. Read all blocks from data nodes
    // 2. Combine blocks in order
    // 3. Write to local file

    // For now, just create an empty local file
    auto local_file = std::ofstream(dst);
    bool success = local_file.is_open();
    local_file.close();
    return success;
  } catch (...)
  {
    return false;
  }
}

bool distributed_file_system::move_to_local(const std::string_view &src, const std::string_view &dst)
{
  // Move = Copy + Delete from DFS
  if (copy_to_local(src, dst))
  {
    return m_name_node->delete_file(std::string(src));
  }
  return false;
}

void distributed_file_system::close()
{
  // just do nothing
}

} // namespace fs
