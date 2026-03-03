#include "core/logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace core
{

typedef std::shared_ptr<spdlog::logger> logger_ptr;

namespace
{
  constexpr inline std::uint64_t MiB = 1024 * 1024;
  static std::string logger_name;

  /// \brief Setup all sinks with same params.
  void setup_sink(const logger_config &cfg, spdlog::sink_ptr &sink)
  {
    if (cfg.pattern.has_value())
      sink->set_pattern(*cfg.pattern);
    if (cfg.log_level)
      sink->set_level(spdlog::level::from_str(*cfg.log_level));
  }
}

void init_logger(const logger_config &cfg)
{
  // Reserve space for logging sinks and save logger_name
  std::vector<spdlog::sink_ptr> sinks;
  sinks.reserve(4);
  logger_name = cfg.name;

  // init thread pool if using async logging
  if (cfg.async.has_value())
    spdlog::init_thread_pool(cfg.async->queue_size, cfg.async->thread_pool_size);

  // create file sink
  if (cfg.rotate.has_value())
    sinks.push_back(std::move(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        cfg.filepath, cfg.rotate->max_size * MiB, cfg.rotate->max_files)));
  else
    sinks.push_back(std::move(std::make_shared<spdlog::sinks::basic_file_sink_mt>(cfg.filepath)));

  // create console sink if use it
  if (cfg.console_output.has_value())
    sinks.push_back(std::move(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()));

  for (auto &sink : sinks)
    setup_sink(cfg, sink);
  logger_ptr logger;

  // create async logger if using async logging
  if (cfg.async.has_value())
    logger = std::make_shared<spdlog::async_logger>(logger_name, //
        sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
  // create logger if not using async logging
  else
    logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());

  // registering logger
  spdlog::register_logger(logger);
}

spdlog::logger &get_logger()
{
  auto logger = spdlog::get(logger_name);
  if (!logger) {
    throw std::runtime_error("Logger not initialized. Call init_logger() first.");
  }
  return *logger;
}

bool is_logger_initialized() noexcept
{
  return spdlog::get(logger_name) != nullptr;
}

} // namespace core
