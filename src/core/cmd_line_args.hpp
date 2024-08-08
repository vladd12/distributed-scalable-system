#pragma once

#include <boost/program_options.hpp>
#include <ostream>
#include <string>
#include <vector>

namespace core
{

namespace opt = ::boost::program_options;

struct cmd_line_args final
{
private:
  opt::options_description m_desc;
  opt::variables_map m_vmap;
  std::vector<std::string> m_unrecognized;

public:
  explicit cmd_line_args(int argc, char **argv);

  bool is_help_need();
  void print_help(std::ostream &stream);

  std::string get_config_filepath() noexcept;
};

} // namespace core
