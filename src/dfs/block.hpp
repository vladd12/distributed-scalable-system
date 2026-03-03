#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace dfs
{

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
  std::unordered_map<std::string, std::string> attributes;
};

} // namespace dfs
