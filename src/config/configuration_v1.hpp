#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace config
{

struct rotate_configuration
{
  std::uint64_t max_size;
  std::uint64_t max_files;

  explicit rotate_configuration(const nlohmann::json &json);
};

struct async_configuration
{
  std::uint64_t thread_pool_size;
  std::uint64_t queue_size;

  explicit async_configuration(const nlohmann::json &json);
};

struct logger_configuration
{
  std::string name;
  std::string filepath;
  std::optional<std::string> pattern;
  std::optional<std::string> log_level;
  std::optional<bool> console_output;
  std::optional<rotate_configuration> rotate;
  std::optional<async_configuration> async;

  explicit logger_configuration(const nlohmann::json &json);
};

struct fs_configuration
{
  std::string host;   ///< The host of the default file system.
                      ///< May be "localhost" or a IP address for DFS.
  std::uint16_t port; ///< The port of the default file system.

  explicit fs_configuration(const nlohmann::json &json);
};

struct file_configuration
{
  std::uint64_t limit; ///< The length limit for downloaded content, in bytes.
                       ///< If this value is larger than zero, content longer
                       ///< than it will be truncated; otherwise (zero or negative),
                       ///< no truncation at all.
  bool is_ignored;     ///< If true, no file content will be saved during fetch.
                       ///< Otherwise file contents will be saved.

  explicit file_configuration(const nlohmann::json &json);
};

struct data_node_configuration
{
  std::string node_id;       ///< Unique identifier for the data node
  std::string host;          ///< Host address of the data node
  std::uint16_t port;        ///< Port number for the data node
  std::uint64_t capacity;    ///< Storage capacity in bytes
  std::string data_dir;      ///< Local directory where this node stores data

  explicit data_node_configuration(const nlohmann::json &json);
};

struct dfs_configuration
{
  std::uint64_t node_port;   ///< The port number that the dfs datanode server uses
                             ///< as a starting point to look for a free port to listen on.
  std::string name_dir;      ///< Determines where on the local filesystem the DFS
                             ///< name node should store the name table.
  std::string data_dir;      ///< Determines where on the local filesystem an DFS data
                             ///< node should store its blocks. If this is a comma- or
                             ///< space-delimited list of directories, then data will be
                             ///< stored in all named directories, typically on different devices.
  std::uint64_t replication; ///< How many copies we try to have at all times. The
                             ///< actual number of replications is at max the number
                             ///< of datanodes in the cluster.
  std::vector<data_node_configuration> data_nodes; ///< Configuration for data nodes

  explicit dfs_configuration(const nlohmann::json &json);
};

struct sort_configuration
{
  std::uint32_t factor; ///< The number of streams to merge at once while sorting files.
                        ///< This determines the number of open file handles.
  std::uint32_t buffer; ///< The total amount of buffer memory for each
                        ///< merge stream to use while sorting files, in MB.

  explicit sort_configuration(const nlohmann::json &json);
};

struct io_configuration
{
  sort_configuration sort;          ///< Sort configuration.
  std::uint64_t file_buffer_size;   ///< The size of buffer for use in sequence files, in bytes.
                                    ///< The size of this buffer should probably be a
                                    ///< multiple of hardware page size.
  std::uint64_t bytes_per_checksum; ///< The number of bytes per checksum.
  std::uint64_t map_index_skip;     ///< Number of index entries to skip between each entry.
  bool skip_checksum_errors;        ///< If true, when a checksum error is encountered while
                                    ///< reading a sequence file, entries are skipped,
                                    ///< instead of throwing an exception.

  explicit io_configuration(const nlohmann::json &json);
};

struct ipc_configuration
{
  std::uint64_t timeout; ///< Defines the timeout for IPC calls, in ms.

  explicit ipc_configuration(const nlohmann::json &json);
};

struct job_tracker_configuration
{
  std::string location;    ///< The host and port that the MapReduce job tracker
                           ///< runs at. If "local", then jobs are run in-process
                           ///< as a single map and reduce task.
  std::uint64_t info_port; ///< The port that the MapReduce job tracker info webserver runs at.

  explicit job_tracker_configuration(const nlohmann::json &json);
};

struct task_tracker_configuration
{
  std::uint64_t output_port;     ///< The port number that the MapReduce task tracker
                                 ///< output server uses as a starting point to
                                 ///< look for a free port to listen on.
  std::uint64_t report_port;     ///< The port number that the MapReduce task tracker
                                 ///< report server uses as a starting point to
                                 ///< look for a free port to listen on.
  std::uint64_t tasks_limit;     ///< The maximum number of tasks that will be
                                 ///< run simultaneously by a task tracker.
  std::uint64_t child_heap_size; ///< The heap size (-Xmx) that will be used
                                 ///< for task tracker child processes.
  std::uint64_t task_timeout;    ///< The number of milliseconds before a task
                                 ///< will be terminated if it neither reads an input,
                                 ///< writes an output, nor updates its status string.

  explicit task_tracker_configuration(const nlohmann::json &json);
};

struct directories_configuration
{
  std::string local;  ///< The local directory where MapReduce stores
                      ///< intermediate data files.  May be a space- or
                      ///< comma- separated list of directories on
                      ///< different devices in order to spread disk i/o.
  std::string system; ///< The shared directory where MapReduce stores control files.
  std::string temp;   ///< A shared directory for temporary files.

  explicit directories_configuration(const nlohmann::json &json);
};

struct mapred_configuration
{
  job_tracker_configuration job_tracker;   ///< Job tracker configuration.
  task_tracker_configuration task_tracker; ///< Task tracker configuration.
  directories_configuration directories;   ///< Configuration for directories.
  std::uint64_t map_tasks;                 ///< The default number of map tasks per job.
                                           ///< Typically set to a prime several times greater
                                           ///< than number of available hosts. Ignored
                                           ///< when "job_tracker.location" is "local".
  std::uint64_t reduce_tasks;              ///< The default number of reduce tasks per job. Typically
                                           ///< set to a prime close to the number of available hosts.
                                           ///< Ignored when "job_tracker.location" is "local".
  std::uint64_t combine_buffer_size;       ///< The number of entries the combining collector
                                           ///< caches before combining them and writing to disk.

  explicit mapred_configuration(const nlohmann::json &json);
};

struct configuration
{
  typedef std::shared_ptr<configuration> ptr;
  logger_configuration logger;
  fs_configuration fs;
  file_configuration file;
  dfs_configuration dfs;
  io_configuration io;
  ipc_configuration ipc;
  mapred_configuration mapred;

  explicit configuration(const nlohmann::json &json);
};

using configuration_ptr = configuration::ptr;

configuration_ptr parse_configuration(const std::string_view &filepath);
configuration_ptr parse_configuration(std::ifstream &file);

} // namespace config
