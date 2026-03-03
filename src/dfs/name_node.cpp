#include "dfs/name_node.hpp"

namespace dfs
{

// Name node implementation
bool name_node::create_file(const std::string &path, bool is_directory)
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

bool name_node::delete_file(const std::string &path)
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

bool name_node::rename_file(const std::string &old_path, const std::string &new_path)
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

file_metadata *name_node::get_metadata(const std::string &path)
{
  std::lock_guard<std::mutex> lock(m_metadata_mutex);

  auto it = m_metadata.find(path);
  if (it == m_metadata.end())
  {
    return nullptr;
  }

  return &it->second;
}

std::vector<std::string> name_node::list_directory(const std::string &path)
{
  std::lock_guard<std::mutex> lock(m_metadata_mutex);

  std::vector<std::string> files;
  std::string prefix = path;
  if (!prefix.empty() && prefix.back() != '/')
  {
    prefix += "/";
  }

  for (const auto &[file_path, metadata] : m_metadata)
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
  for (const auto &[node_id, node] : m_data_nodes)
  {
    if (node.is_alive)
    {
      available_nodes.push_back(node);
    }
  }

  // Sort by available space (descending)
  std::sort(available_nodes.begin(), available_nodes.end(),
      [](const data_node &a, const data_node &b) { return (a.capacity - a.used_space) > (b.capacity - b.used_space); });

  if (available_nodes.size() > count)
  {
    available_nodes.resize(count);
  }

  return available_nodes;
}

void name_node::register_data_node(const data_node &node)
{
  std::lock_guard<std::mutex> lock(m_nodes_mutex);
  m_data_nodes[node.node_id] = node;
}

void name_node::update_heartbeat(const std::string &node_id)
{
  std::lock_guard<std::mutex> lock(m_nodes_mutex);

  auto it = m_data_nodes.find(node_id);
  if (it != m_data_nodes.end())
  {
    it->second.last_heartbeat = std::chrono::system_clock::now();
    it->second.is_alive = true;
  }
}

} // namespace dfs
