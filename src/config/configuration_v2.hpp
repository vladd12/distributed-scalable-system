#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>

namespace detail
{
using njson = nlohmann::json;

template <typename T, bool required> //
struct required_datatype_selector;

template <typename T> //
struct required_datatype_selector<T, true>
{
  typedef T type;
};

template <typename T> //
struct required_datatype_selector<T, false>
{
  typedef std::optional<T> type;
};

template <typename T, bool required> //
class json_field
{
private:
};

} // namespace detail

namespace config
{

} // namespace config
