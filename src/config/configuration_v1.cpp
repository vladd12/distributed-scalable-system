#include "config/configuration_v1.hpp"

#include <nlohmann/json.hpp>

namespace config
{

rotate_configuration::rotate_configuration(const nlohmann::json &json) //
    : max_size(json["max-size"]), max_files(json["max-files"])
{
}

async_configuration::async_configuration(const nlohmann::json &json) //
    : thread_pool_size(json["thread-pool-size"]), queue_size(json["queue-size"])
{
}

logger_configuration::logger_configuration(const nlohmann::json &json) //
    : name(json["name"]), filepath(json["filepath"])
{
  if (json.contains("pattern"))
    pattern.emplace(std::move(std::string(json["pattern"])));
  if (json.contains("log-level"))
    log_level.emplace(std::move(std::string(json["log-level"])));
  if (json.contains("console-output"))
    console_output.emplace(bool(json["console-output"]));
  if (json.contains("rotate"))
    rotate.emplace(std::move(rotate_configuration(json["rotate"])));
  if (json.contains("async"))
    async.emplace(std::move(async_configuration(json["async"])));
}

fs_configuration::fs_configuration(const nlohmann::json &json) //
    : host(json["host"]), port(json["port"])
{
}

file_configuration::file_configuration(const nlohmann::json &json) //
    : limit(json["limit"]), is_ignored(json["ignored"])
{
}

data_node_configuration::data_node_configuration(const nlohmann::json &json) //
    : node_id(json["node-id"])
    , host(json["host"])
    , port(json["port"])
    , capacity(json["capacity"])
    , data_dir(json["data-dir"])
{
}

dfs_configuration::dfs_configuration(const nlohmann::json &json) //
    : node_port(json["node-port"])
    , name_dir(json["name-dir"])
    , data_dir(json["data-dir"])
    , replication(json["replication"])
{
  if (json.contains("data-nodes"))
  {
    for (const auto& node_json : json["data-nodes"])
    {
      data_nodes.emplace_back(node_json);
    }
  }
}

sort_configuration::sort_configuration(const nlohmann::json &json) //
    : factor(json["factor"]), buffer(json["buffer"])
{
}

io_configuration::io_configuration(const nlohmann::json &json) //
    : sort(json["sort"])
    , file_buffer_size(json["file-buffer-size"])
    , bytes_per_checksum(json["bytes-per-checksum"])
    , map_index_skip(json["map-index-skip"])
    , skip_checksum_errors(json["skip-checksum-errors"])
{
}

ipc_configuration::ipc_configuration(const nlohmann::json &json) : timeout(json["timeout"])
{
}

job_tracker_configuration::job_tracker_configuration(const nlohmann::json &json) //
    : location(json["location"]), info_port(json["info-port"])
{
}

task_tracker_configuration::task_tracker_configuration(const nlohmann::json &json) //
    : output_port(json["output-port"])
    , report_port(json["report-port"])
    , tasks_limit(json["tasks-limit"])
    , child_heap_size(json["child-heap-size"])
    , task_timeout(json["task-timeout"])
{
}

directories_configuration::directories_configuration(const nlohmann::json &json) //
    : local(json["local"]), system(json["system"]), temp(json["temp"])
{
}

mapred_configuration::mapred_configuration(const nlohmann::json &json) //
    : job_tracker(json["job-tracker"])
    , task_tracker(json["task-tracker"])
    , directories(json["directories"])
    , map_tasks(json["map-tasks"])
    , reduce_tasks(json["reduce-tasks"])
    , combine_buffer_size(json["combine-buffer-size"])
{
}

configuration::configuration(const nlohmann::json &json) //
    : logger(json["logger"])
    , fs(json["fs"])
    , file(json["file"])
    , dfs(json["dfs"])
    , io(json["io"])
    , ipc(json["ipc"])
    , mapred(json["map-reduce"])
{
}

configuration_ptr parse_configuration(const std::string_view &filepath)
{
  std::ifstream file(filepath.data());
  return parse_configuration(file);
}

configuration_ptr parse_configuration(std::ifstream &file)
{
  const nlohmann::json data = nlohmann::json::parse(file, nullptr, true, true);
  return std::make_unique<configuration>(data);
}

} // namespace config
