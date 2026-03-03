#include <config/configuration_v1.hpp>
#include <core/cmd_line_args.hpp>
#include <core/errors.hpp>
#include <core/logger.hpp>
#include <fs/file_system_provider.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
  try
  {
    core::cmd_line_args arguments { argc, argv };
    if (arguments.is_help_need())
    {
      arguments.print_help(std::cout);
      return 2;
    }
    auto cfg_ptr { config::parse_configuration(arguments.get_config_filepath()) };
    core::init_logger(cfg_ptr->logger);
    core::get_logger().info("Logger init is successfully");

    fs::file_system_provider fs_provider { cfg_ptr };
    auto fs = fs_provider.get(cfg_ptr->fs.host, cfg_ptr->fs.port);
    fs->close();
  } catch (const spdlog::spdlog_ex &ex)
  {
    std::cout << "Log init failed: " << ex.what() << std::endl;
    return 2;
  } catch (const core::not_implemented_error &ex)
  {
    std::cout << "Not implemented feature: " << ex.what() << std::endl;
    return 3;
  } catch (std::exception &ex)
  {
    std::cout << "Runtime error: " << ex.what() << '\n';
    return 1;
  } catch (...)
  {
    std::cout << "unrecognized error\n";
    return 1;
  }

  return 0;
}
