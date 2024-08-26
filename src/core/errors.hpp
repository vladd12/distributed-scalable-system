#pragma once

#include <stdexcept>

namespace core
{

/// \brief Class for not implemented features.
class not_implemented_error : public std::runtime_error
{
};

} // namespace core
