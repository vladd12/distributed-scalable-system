#pragma once

#include <dfs/block.hpp>
#include <dfs/data_node.hpp>
//
#include <random>

namespace dfs
{

// Simple name node implementation
class name_node
{
private:
  std::unordered_map<std::string, file_metadata> m_metadata;
  std::unordered_map<std::string, data_node> m_data_nodes;
  std::unordered_map<std::string, std::mutex> m_file_locks;
  std::mutex m_metadata_mutex;
  std::mutex m_nodes_mutex;
  std::random_device m_rd;
  std::mt19937 m_gen;

  std::size_t m_replication_factor;
  static constexpr std::size_t BLOCK_SIZE = 64 * 1024 * 1024; // 64MB

public:
  explicit name_node(std::size_t replication_factor) : m_gen(m_rd()), m_replication_factor(replication_factor)
  {
  }

  bool create_file(const std::string &path, bool is_directory = false);
  bool delete_file(const std::string &path);
  bool rename_file(const std::string &old_path, const std::string &new_path);
  file_metadata *get_metadata(const std::string &path);
  std::vector<std::string> list_directory(const std::string &path);
  std::vector<data_node> select_data_nodes_for_write(std::size_t count = 0);
  void register_data_node(const data_node &node);
  void update_heartbeat(const std::string &node_id);
};

} // namespace dfs
