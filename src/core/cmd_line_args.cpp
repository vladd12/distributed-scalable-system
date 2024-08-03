#include "core/cmd_line_args.hpp"

namespace core
{

cmd_line_args::cmd_line_args(int argc, char **argv) : m_desc("All options")
{
    m_desc.add_options()                                                                                  //
        ("help", "show daemon's options")                                                                 //
        ("config", opt::value<std::string>()->default_value("default_config.json"), "config for daemon"); //
    auto parsed { std::move(opt::parse_command_line(argc, argv, m_desc)) };
    m_unrecognized = std::move(opt::collect_unrecognized(parsed.options, //
        opt::collect_unrecognized_mode::include_positional));            //
    opt::store(parsed, m_vmap);
    opt::notify(m_vmap);
}

bool cmd_line_args::is_help_need()
{
    return m_vmap.count("help") || m_unrecognized.size() > 1;
}

void cmd_line_args::print_help(std::ostream &stream)
{
    stream << m_desc << '\n';
    if (m_unrecognized.size() > 1)
    {
        stream << "unrecognized params:\n";
        for (auto &&param : m_unrecognized)
            stream << param << ' ';
        stream << '\n';
    }
}

std::string cmd_line_args::get_config_filepath() noexcept
{
    return m_vmap["config"].as<std::string>();
}

} // namespace core
