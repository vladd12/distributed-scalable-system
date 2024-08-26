#pragma once

#include <config/configuration_v1.hpp>
#include <spdlog/logger.h>

namespace core
{

typedef ::config::logger_configuration logger_config;

/// \brief Creates logger.
void init_logger(const logger_config &cfg);

/// \brief Returns created logger.
spdlog::logger &get_logger();

} // namespace core
