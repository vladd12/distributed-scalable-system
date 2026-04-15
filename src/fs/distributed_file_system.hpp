#pragma once

#include <config/configuration.hpp>
#include <dfs/name_node.hpp>
#include <fs/file_system.hpp>
//
#include <mutex>
//
#include <boost/asio.hpp>

namespace fs
{

class distributed_file_system final : public file_system
{
private:
  std::unique_ptr<dfs::name_node> m_name_node;
  boost::asio::io_context m_io_context;
  std::unordered_map<std::string, std::unique_ptr<std::mutex>> m_path_locks;
  std::mutex m_locks_mutex;
  config::name_node_configuration m_name_node_config;

  // Helper methods
  std::string generate_block_id();
  bool transfer_data_to_nodes(
      const std::vector<dfs::data_node> &nodes, const std::string &block_id, const std::vector<char> &data);
  std::vector<char> read_data_from_nodes(const dfs::file_block &block);
  bool replicate_block(const std::string &block_id, const std::vector<dfs::data_node> &target_nodes);

public:
  explicit distributed_file_system(const config::name_node_configuration &name_node_config);
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
