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
  using stored_type = detail::required_datatype_selector_t<array_type, required>;

protected:
  stored_type field;

public:
  inline json_field_base() noexcept
  {
  }

  inline json_field_base(const stored_type &data) : field(data)
  {
  }

  inline json_field_base(stored_type &&data) : field(std::move(data))
  {
  }

  inline json_field_base(const json_field_base &rhs) //
      noexcept(noexcept(stored_type(std::declval<const stored_type &>())))
      : field(rhs.field)
  {
  }

  inline json_field_base(json_field_base &&rhs) //
      noexcept(noexcept(stored_type(std::declval<stored_type &&>())))
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

  /// \brief Conversion operator for not optional and not array stored data types.
  template <typename R = std::enable_if_t<required && !std::is_array_v<T>, stored_type>>
  [[nodiscard]] inline operator R() const noexcept
  {
    return field;
  }

  /// \brief Conversion operator for not optional and not array stored data types.
  template <typename R = std::enable_if_t<required && !std::is_array_v<T>, stored_type>>
  [[nodiscard]] inline operator R &() noexcept
  {
    return field;
  }

  /// \brief Conversion operator for not optional and not array stored data types.
  template <typename R = std::enable_if_t<required && !std::is_array_v<T>, stored_type>>
  [[nodiscard]] inline operator const R &() const noexcept
  {
    return field;
  }

  [[nodiscard]] constexpr inline bool has_value() const noexcept
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
      : base_type(base_type::stored_type(data[base_type::field_name]))                      //
  {
  }

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element>, bool> = true> //
  inline explicit json_field_required(njson &&data)                                         //
      : base_type(std::move(base_type::stored_type(data[base_type::field_name])))           //
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
