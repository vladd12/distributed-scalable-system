#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

namespace config
{

struct configuration
{
    struct fs_configuration
    {
        std::string name; ///< The name of the default file system.
                          ///< May be "local" or a host:port for DFS.
    } fs;
    struct file_configuration
    {
        std::uint64_t limit; ///< The length limit for downloaded content, in bytes.
                             ///< If this value is larger than zero, content longer
                             ///< than it will be truncated; otherwise (zero or negative),
                             ///< no truncation at all.
                             ///
        bool is_ignored;     ///< If true, no file content will be saved during fetch.
                             ///< Otherwise file contents will be saved.
    } file;
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
    } dfs;
    struct io_configuration
    {
        struct sort_configuration
        {
            std::uint32_t factor; ///< The number of streams to merge at once while sorting files.
                                  ///< This determines the number of open file handles.
            std::uint32_t buffer; ///< The total amount of buffer memory for each
                                  ///< merge stream to use while sorting files, in MB.
        } sort;
        std::uint64_t file_buffer_size;   ///< The size of buffer for use in sequence files, in bytes.
                                          ///< The size of this buffer should probably be a
                                          ///< multiple of hardware page size.
        std::uint64_t bytes_per_checksum; ///< The number of bytes per checksum.
        std::uint64_t map_index_skip;     ///< Number of index entries to skip between each entry.
        bool skip_checksum_errors;        ///< If true, when a checksum error is encountered while
                                          ///< reading a sequence file, entries are skipped,
                                          ///< instead of throwing an exception.
    } io;
    struct ipc_configuration
    {
        std::uint64_t timeout; ///< Defines the timeout for IPC calls, in ms.
    } ipc;
    struct mapred_configuration
    {
        struct job_tracker_configuration
        {
            std::string location;    ///< The host and port that the MapReduce job tracker
                                     ///< runs at. If "local", then jobs are run in-process
                                     ///< as a single map and reduce task.
            std::uint64_t info_port; /// The port that the MapReduce job tracker info webserver runs at.
        } job_tracker;
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

        } task_tracker;
        struct directories_configuration
        {
            std::string local;  ///< The local directory where MapReduce stores
                                ///< intermediate data files.  May be a space- or
                                ///< comma- separated list of directories on
                                ///< different devices in order to spread disk i/o.
            std::string system; ///< The shared directory where MapReduce stores control files.
            std::string temp;   ///< A shared directory for temporary files.
        } directories;
        std::uint64_t map_tasks;           ///< The default number of map tasks per job.
                                           ///< Typically set to a prime several times greater
                                           ///< than number of available hosts. Ignored
                                           ///< when "job_tracker.location" is "local".
        std::uint64_t reduce_tasks;        ///< The default number of reduce tasks per job. Typically
                                           ///< set to a prime close to the number of available hosts.
                                           ///< Ignored when "job_tracker.location" is "local".
        std::uint64_t combine_buffer_size; ///< The number of entries the combining collector
                                           ///< caches before combining them and writing to disk.
    } mapred;
};

using configuration_ptr = std::unique_ptr<configuration>;

configuration_ptr parse_configuration(const std::string_view &filepath);
configuration_ptr parse_configuration(std::ifstream &file);

} // namespace config
