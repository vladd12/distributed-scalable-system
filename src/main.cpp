#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    // create a color multi-threaded logger
    auto console = spdlog::stdout_color_mt("console");
    auto err_logger = spdlog::stderr_color_mt("stderr");
    console->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name)");
    err_logger->critical("some error");
    return 0;
}
