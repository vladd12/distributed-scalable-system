#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <config/configuration_v1.hpp>
#include <fs/file_system.hpp>
#include <map>
#include <mutex>
#include <random>
#include <unordered_map>

namespace fs
{

// Forward declarations
struct file_metadata;
struct data_node;
class name_node;

// File block information
struct file_block
{
  std::string block_id;
  std::uint64_t size;
  std::vector<std::string> replicas; // Data node IDs where block is stored
  std::string checksum;
};

// File metadata stored in name node
struct file_metadata
{
  std::string path;
  bool is_directory;
  std::uint64_t size;
  std::chrono::system_clock::time_point created_time;
  std::chrono::system_clock::time_point modified_time;
  std::vector<file_block> blocks;
  std::map<std::string, std::string> attributes;
};

// Data node information
struct data_node
{
  std::string node_id;
  std::string host;
  std::uint16_t port;
  std::uint64_t capacity;
  std::uint64_t used_space;
  bool is_alive;
  std::chrono::system_clock::time_point last_heartbeat;
};

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

class distributed_file_system final : public file_system
{
private:
  std::unique_ptr<name_node> m_name_node;
  boost::asio::io_context m_io_context;
  std::unordered_map<std::string, std::unique_ptr<std::mutex>> m_path_locks;
  std::mutex m_locks_mutex;
  config::dfs_configuration m_dfs_config;

  // Helper methods
  std::string generate_block_id();
  bool transfer_data_to_nodes(
      const std::vector<data_node> &nodes, const std::string &block_id, const std::vector<char> &data);
  std::vector<char> read_data_from_nodes(const file_block &block);
  bool replicate_block(const std::string &block_id, const std::vector<data_node> &target_nodes);

public:
  explicit distributed_file_system(const config::dfs_configuration &dfs_config);
  ~distributed_file_system() = default;
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
