#include "fs/distributed_file_system.hpp"

#include <core/errors.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace fs
{

static const core::not_implemented_error err("func is not implemented yet");

// Name node implementation
bool name_node::create_file(const std::string& path, bool is_directory)
{
  std::lock_guard<std::mutex> lock(m_metadata_mutex);
  
  if (m_metadata.find(path) != m_metadata.end())
  {
    return false; // File already exists
  }
  
  file_metadata metadata;
  metadata.path = path;
  metadata.is_directory = is_directory;
  metadata.size = 0;
  metadata.created_time = std::chrono::system_clock::now();
  metadata.modified_time = metadata.created_time;
  
  m_metadata[path] = std::move(metadata);
  return true;
}

bool name_node::delete_file(const std::string& path)
{
  std::lock_guard<std::mutex> lock(m_metadata_mutex);
  
  auto it = m_metadata.find(path);
  if (it == m_metadata.end())
  {
    return false; // File not found
  }
  
  // TODO: Clean up blocks on data nodes
  m_metadata.erase(it);
  return true;
}

bool name_node::rename_file(const std::string& old_path, const std::string& new_path)
{
  std::lock_guard<std::mutex> lock(m_metadata_mutex);
  
  auto it = m_metadata.find(old_path);
  if (it == m_metadata.end())
  {
    return false; // File not found
  }
  
  if (m_metadata.find(new_path) != m_metadata.end())
  {
    return false; // Target path already exists
  }
  
  file_metadata metadata = std::move(it->second);
  metadata.path = new_path;
  metadata.modified_time = std::chrono::system_clock::now();
  
  m_metadata.erase(it);
  m_metadata[new_path] = std::move(metadata);
  return true;
}

file_metadata* name_node::get_metadata(const std::string& path)
{
  std::lock_guard<std::mutex> lock(m_metadata_mutex);
  
  auto it = m_metadata.find(path);
  if (it == m_metadata.end())
  {
    return nullptr;
  }
  
  return &it->second;
}

std::vector<std::string> name_node::list_directory(const std::string& path)
{
  std::lock_guard<std::mutex> lock(m_metadata_mutex);
  
  std::vector<std::string> files;
  std::string prefix = path;
  if (!prefix.empty() && prefix.back() != '/')
  {
    prefix += "/";
  }
  
  for (const auto& [file_path, metadata] : m_metadata)
  {
    if (file_path.starts_with(prefix) && file_path != path)
    {
      std::string relative = file_path.substr(prefix.length());
      if (relative.find('/') == std::string::npos) // Direct child
      {
        files.push_back(relative);
      }
    }
  }
  
  return files;
}

std::vector<data_node> name_node::select_data_nodes_for_write(std::size_t count)
{
  std::lock_guard<std::mutex> lock(m_nodes_mutex);
  
  // Use replication factor if count is 0
  if (count == 0)
  {
    count = m_replication_factor;
  }
  
  std::vector<data_node> available_nodes;
  for (const auto& [node_id, node] : m_data_nodes)
  {
    if (node.is_alive)
    {
      available_nodes.push_back(node);
    }
  }
  
  // Sort by available space (descending)
  std::sort(available_nodes.begin(), available_nodes.end(),
    [](const data_node& a, const data_node& b) {
      return (a.capacity - a.used_space) > (b.capacity - b.used_space);
    });
  
  if (available_nodes.size() > count)
  {
    available_nodes.resize(count);
  }
  
  return available_nodes;
}

void name_node::register_data_node(const data_node& node)
{
  std::lock_guard<std::mutex> lock(m_nodes_mutex);
  m_data_nodes[node.node_id] = node;
}

void name_node::update_heartbeat(const std::string& node_id)
{
  std::lock_guard<std::mutex> lock(m_nodes_mutex);
  
  auto it = m_data_nodes.find(node_id);
  if (it != m_data_nodes.end())
  {
    it->second.last_heartbeat = std::chrono::system_clock::now();
    it->second.is_alive = true;
  }
}

// Distributed file system implementation
distributed_file_system::distributed_file_system(const config::dfs_configuration& dfs_config)
  : m_name_node(std::make_unique<name_node>(dfs_config.replication)),
    m_dfs_config(dfs_config)
{
  // Validate configuration
  if (m_dfs_config.data_nodes.empty())
  {
    throw std::invalid_argument("No data nodes configured. A distributed file system requires at least one data node.");
  }
  
  if (m_dfs_config.replication > m_dfs_config.data_nodes.size())
  {
    throw std::invalid_argument("Replication factor cannot be greater than the number of data nodes.");
  }
  
  // Initialize data nodes from configuration
  for (const auto& node_config : m_dfs_config.data_nodes)
  {
    data_node node;
    node.node_id = node_config.node_id;
    node.host = node_config.host;
    node.port = node_config.port;
    node.capacity = node_config.capacity;
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

bool distributed_file_system::transfer_data_to_nodes(const std::vector<data_node>& nodes, const std::string& block_id, const std::vector<char>& data)
{
  // In a real implementation, this would use network communication to transfer data to nodes
  // For now, simulate successful transfer
  return !nodes.empty();
}

std::vector<char> distributed_file_system::read_data_from_nodes(const file_block& block)
{
  // In a real implementation, this would read data from the data nodes
  // For now, return empty data
  return std::vector<char>();
}

bool distributed_file_system::replicate_block(const std::string& block_id, const std::vector<data_node>& target_nodes)
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

bool distributed_file_system::is_exists(const std::string_view &path) noexcept
{
  try
  {
    const auto metadata = m_name_node->get_metadata(std::string(path));
    return metadata != nullptr;
  }
  catch (...)
  {
    return false;
  }
}

bool distributed_file_system::is_directory(const std::string_view &path) noexcept
{
  try
  {
    const auto metadata = m_name_node->get_metadata(std::string(path));
    return metadata && metadata->is_directory;
  }
  catch (...)
  {
    return false;
  }
}

std::uint64_t distributed_file_system::size(const std::string_view &path) noexcept
{
  try
  {
    const auto metadata = m_name_node->get_metadata(std::string(path));
    return metadata ? metadata->size : 0;
  }
  catch (...)
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
  }
  catch (...)
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
  }
  catch (...)
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
    }
    catch (...)
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
  }
  catch (...)
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
