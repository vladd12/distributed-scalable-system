#include "config/configuration.hpp"

#include <nlohmann/json.hpp>

namespace config
{

configuration_ptr parse_configuration(const std::string_view &filepath)
{
    std::ifstream file(filepath.data());
    return parse_configuration(file);
}

configuration_ptr parse_configuration(std::ifstream &file)
{
    nlohmann::json data = nlohmann::json::parse(file, nullptr, true, true);
    return configuration_ptr { //
        new configuration {
            .fs {
                .name = data["fs"]["name"], //
            },
            .file {
                .limit = data["file"]["limit"],        //
                .is_ignored = data["file"]["ignored"], //
            },
            .dfs {
                .node_port = data["dfs"]["node-port"],     //
                .name_dir = data["dfs"]["name-dir"],       //
                .data_dir = data["dfs"]["data-dir"],       //
                .replication = data["dfs"]["replication"], //
            },
            .io {
                .sort {
                    .factor = data["io"]["sort"]["factor"], //
                    .buffer = data["io"]["sort"]["buffer"], //
                },
                .file_buffer_size = data["io"]["file-buffer-size"],         //
                .bytes_per_checksum = data["io"]["bytes-per-checksum"],     //
                .map_index_skip = data["io"]["map-index-skip"],             //
                .skip_checksum_errors = data["io"]["skip-checksum-errors"], //
            },
            .ipc {
                .timeout = data["ipc"]["timeout"], //
            },
            .mapred {
                .job_tracker {
                    .location = data["map-reduce"]["job-tracker"]["location"],   //
                    .info_port = data["map-reduce"]["job-tracker"]["info-port"], //
                },
                .task_tracker {
                    .output_port = data["map-reduce"]["task-tracker"]["output-port"],         //
                    .report_port = data["map-reduce"]["task-tracker"]["report-port"],         //
                    .tasks_limit = data["map-reduce"]["task-tracker"]["tasks-limit"],         //
                    .child_heap_size = data["map-reduce"]["task-tracker"]["child-heap-size"], //
                    .task_timeout = data["map-reduce"]["task-tracker"]["task-timeout"],       //
                },
                .directories {
                    .local = data["map-reduce"]["directories"]["local"],   //
                    .system = data["map-reduce"]["directories"]["system"], //
                    .temp = data["map-reduce"]["directories"]["temp"],     //
                },
                .map_tasks = data["map-reduce"]["map-tasks"],                     //
                .reduce_tasks = data["map-reduce"]["reduce-tasks"],               //
                .combine_buffer_size = data["map-reduce"]["combine-buffer-size"], //
            },
        }
    };
}

} // namespace config
