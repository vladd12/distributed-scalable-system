#pragma once

#include <optional>
#include <type_traits>
#include <vector>

namespace detail
{

template <typename T> //
struct array_datatype_checker
{
  typedef std::conditional_t<std::is_array_v<T>, std::vector<std::remove_extent_t<T>>, T> type;
};

template <typename T> //
using array_datatype_checker_t = typename array_datatype_checker<T>::type;

template <typename T, bool required> //
struct required_datatype_selector;

template <typename T> //
struct required_datatype_selector<T, true>
{
  typedef array_datatype_checker_t<T> type;
};

template <typename T> //
struct required_datatype_selector<T, false>
{
  typedef std::optional<array_datatype_checker_t<T>> type;
};

template <typename T, bool required> //
using required_datatype_selector_t = typename required_datatype_selector<T, required>::type;

template <typename... Ts> //
struct type_holder
{
};

// explicit deduction guide (not needed)
template <typename... Ts> type_holder(Ts...) -> type_holder<Ts...>;

} // namespace detail
