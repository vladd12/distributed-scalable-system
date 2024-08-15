#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace detail
{
using njson = nlohmann::json;

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

template <typename T, auto &name, bool required> //
class json_field_base
{
public:
  static constexpr std::string_view field_name = std::string_view(name);
  using value_type = required_datatype_selector_t<T, required>;
  using element_type = std::remove_cvref_t<std::remove_extent_t<T>>;

protected:
  value_type field;

public:
  inline json_field_base() noexcept
  {
  }

  inline json_field_base(const value_type &data) : field(data)
  {
  }

  inline json_field_base(value_type &&data) : field(std::move(data))
  {
  }

  inline json_field_base(const json_field_base &rhs) //
      noexcept(noexcept(value_type(std::declval<const value_type &>())))
      : field(rhs.field)
  {
  }

  inline json_field_base(json_field_base &&rhs) //
      noexcept(noexcept(value_type(std::declval<value_type &&>())))
      : field(std::move(rhs.field))
  {
  }

  inline json_field_base &operator=(const json_field_base &rhs)
  {
    field = rhs.field;
  }

  inline json_field_base &operator=(json_field_base &&rhs)
  {
    field = std::move(rhs.field);
  }

  [[nodiscard]] inline operator value_type() const noexcept
  {
    return field;
  }

  [[nodiscard]] inline operator value_type &() noexcept
  {
    return field;
  }

  [[nodiscard]] inline operator const value_type &() const noexcept
  {
    return field;
  }

  [[nodiscard]] constexpr inline bool access() const noexcept
  {
    if constexpr (required)
      return true;
    else
      return field.has_value();
  }
};

template <typename T, auto &name> //
struct json_field_required : public json_field_base<T, name, true>
{
public:
  typedef json_field_base<T, name, true> base_type;

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element>, bool> = true> //
  inline explicit json_field_required(const njson &data)                                    //
      : base_type(base_type::value_type(data[base_type::field_name]))                       //
  {
  }

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element>, bool> = true> //
  inline explicit json_field_required(njson &&data)                                         //
      : base_type(std::move(base_type::value_type(data[base_type::field_name])))            //
  {
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array>, bool> = true> //
  inline explicit json_field_required(const njson &data)                               //
  {
    const auto &array = data[base_type::field_name];
    auto data_begin = array.begin();
    auto data_end = array.end();
    this->field.reserve(data_end - data_begin);
    for (auto iter = data_begin; iter != data_end; ++iter)
      this->field.push_back(std::move(base_type::element_type(*iter)));
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array>, bool> = true> //
  inline explicit json_field_required(njson &&data)                                    //
  {
    auto &array = data[base_type::field_name];
    auto data_begin = array.begin();
    auto data_end = array.end();
    this->field.reserve(data_end - data_begin);
    for (auto iter = data_begin; iter != data_end; ++iter)
      this->field.push_back(std::move(base_type::element_type(std::move(*iter))));
  }
};

template <typename T, auto &name> //
struct json_field_optional : public json_field_base<T, name, false>
{
public:
  typedef json_field_base<T, name, true> base_type;

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element>, bool> = true> //
  inline explicit json_field_optional(const njson &data)                                    //
  {
    if (data.contains(base_type::field_name))
      this->field = data[base_type::field_name];
  }

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element>, bool> = true> //
  inline explicit json_field_optional(njson &&data)                                         //
  {
    if (data.contains(base_type::field_name))
      this->field = std::move(data[base_type::field_name]);
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array>, bool> = true> //
  inline explicit json_field_optional(const njson &data)                               //
  {
    if (data.contains(base_type::field_name))
    {
      const auto &array = data[base_type::field_name];
      auto data_begin = array.begin();
      auto data_end = array.end();
      this->field->reserve(data_end - data_begin);
      for (auto iter = data_begin; iter != data_end; ++iter)
        this->field->push_back(std::move(base_type::element_type(*iter)));
    }
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array>, bool> = true> //
  inline explicit json_field_optional(njson &&data)                                    //
  {
    if (data.contains(base_type::field_name))
    {
      auto &array = data[base_type::field_name];
      auto data_begin = array.begin();
      auto data_end = array.end();
      this->field->reserve(data_end - data_begin);
      for (auto iter = data_begin; iter != data_end; ++iter)
        this->field->push_back(std::move(base_type::element_type(std::move(*iter))));
    }
  }
};

template <typename T, auto &name> //
struct json_struct : public T
{
public:
  static constexpr std::string_view struct_name = std::string_view(name);

  template <typename... Ts>                           //
  inline explicit json_struct(const njson &data,      //
      [[maybe_unused]] type_holder<Ts...>)            //
      : T(std::forward<Ts>(Ts(data[struct_name]))...) //
  {
  }

  template <typename... Ts>                                      //
  inline explicit json_struct(njson &&data,                      //
      [[maybe_unused]] type_holder<Ts...>)                       //
      : T(std::forward<Ts>(Ts(std::move(data[struct_name])))...) //
  {
  }

  inline explicit json_struct(const njson &data) //
      : json_struct(data, T::holder)             //
  {
  }

  inline explicit json_struct(njson &&data)     //
      : json_struct(std::move(data), T::holder) //
  {
  }
};

template <typename T> //
struct json_unnamed_struct : public T
{
public:
  template <typename... Ts>                              //
  inline explicit json_unnamed_struct(const njson &data, //
      [[maybe_unused]] type_holder<Ts...>)               //
      : T(std::move(std::forward<Ts>(Ts(data))...))      //
  {
  }

  template <typename... Ts>                                    //
  inline explicit json_unnamed_struct(njson &&data,            //
      [[maybe_unused]] type_holder<Ts...>)                     //
      : T(std::move(std::forward<Ts>(Ts(std::move(data)))...)) //
  {
  }

  inline explicit json_unnamed_struct(const njson &data) //
      : json_unnamed_struct(data, T::holder)             //
  {
  }

  inline explicit json_unnamed_struct(njson &&data)     //
      : json_unnamed_struct(std::move(data), T::holder) //
  {
  }
};

} // namespace detail

namespace config
{

void foo(const detail::njson &data);

} // namespace config

namespace test
{

void test01();

} // namespace test
