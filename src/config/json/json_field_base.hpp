#pragma once

#include <config/json/detail/type_traits.hpp>
#include <nlohmann/json.hpp>
#include <string_view>

namespace json
{
using njson = nlohmann::json;

template <typename T, auto &name, bool required> //
class json_field_base
{
public:
  static constexpr std::string_view field_name = std::string_view(name);
  using array_type = detail::array_datatype_checker_t<std::remove_cvref_t<T>>;
  using clean_type = std::remove_cvref_t<std::remove_extent_t<T>>;
  using value_type = detail::required_datatype_selector_t<T, required>;

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

} // namespace json
