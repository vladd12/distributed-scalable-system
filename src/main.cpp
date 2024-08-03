#include <config/configuration.hpp>
#include <core/cmd_line_args.hpp>
#include <iostream>

void test(config::configuration_ptr &cfg)
{
    std::cout << cfg->dfs.name_dir << ' ' << cfg->dfs.data_dir << '\n';
}

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
        test(cfg);
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
