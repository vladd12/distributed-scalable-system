#pragma once

#include <config/configuration_v1.hpp>
#include <spdlog/logger.h>

namespace core
{

using logger_config = ::config::logger_configuration;

/// \brief Creates and initializes the global logger
/// \param cfg Logger configuration
/// \throws spdlog::spdlog_ex if logger initialization fails
void init_logger(const logger_config &cfg);

/// \brief Returns the initialized logger instance
/// \return Reference to the global logger
/// \throws std::runtime_error if logger hasn't been initialized
spdlog::logger &get_logger();

/// \brief Checks if logger has been initialized
/// \return true if logger is initialized, false otherwise
bool is_logger_initialized() noexcept;

} // namespace core
