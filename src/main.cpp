#include <core/cmd_line_args.hpp>
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
    } catch (std::exception &e)
    {
        std::cout << e.what() << '\n';
        return 1;
    } catch (...)
    {
        std::cout << "unrecognized error\n";
        return 1;
    }

    return 0;
}
