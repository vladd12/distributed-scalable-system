#include <config/configuration_v1.hpp>
#include <core/cmd_line_args.hpp>
#include <core/logger.hpp>
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
    auto cfg { config::parse_configuration(arguments.get_config_filepath()) };
    core::init_logger(cfg->logger);
  } catch (std::exception &e)
  {
    std::cout << "Runtime error: " << e.what() << '\n';
    return 1;
  } catch (...)
  {
    std::cout << "unrecognized error\n";
    return 1;
  }

  return 0;
}
