#pragma once

#include <config/configuration_v1.hpp>
#include <spdlog/logger.h>

namespace core
{

typedef ::config::logger_configuration logger_config;

void init_logger(const logger_config &cfg);

spdlog::logger &get_logger();

} // namespace core
