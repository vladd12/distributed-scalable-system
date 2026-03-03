#pragma once

#include <chrono>
#include <string>

namespace dfs
{

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

} // namespace dfs
