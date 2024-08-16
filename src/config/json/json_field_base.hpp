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
  // Static type instance restrictions
  static_assert(!std::is_const_v<T>, "Must be not a const type");
  static_assert(!std::is_reference_v<T>, "Must be not a reference type");
  static_assert(!std::is_rvalue_reference_v<T>, "Must be not a rvalue reference type");

  static constexpr std::string_view field_name = std::string_view(name); ///< json field name
  using clean_type = std::remove_cvref_t<std::remove_extent_t<T>>;

  static constexpr auto is_optional = !required;
  static constexpr auto is_array = std::is_array_v<std::remove_cvref_t<T>>;

  // stored_type is type for these cases:
  // std::optional<std::vector<T>> for <is_optional = true,  is_array = true>
  // std::optional<T>              for <is_optional = true,  is_array = false>
  // std::vector<T>                for <is_optional = false, is_array = true>
  // T                             for <is_optional = false, is_array = false>
  using stored_type = std::conditional_t<is_optional,                                                  //
      std::conditional_t<is_array, std::optional<std::vector<clean_type>>, std::optional<clean_type>>, //
      std::conditional_t<is_array, std::vector<clean_type>, clean_type>>;

  // external_type is type for external acces to stored field data
  using external_type = std::conditional_t<is_array, std::vector<clean_type>, clean_type>;

protected:
  stored_type field;

public:
  /// \brief Default ctor.
  inline json_field_base() noexcept
  {
  }

  /// \brief Copying ctor for external type.
  inline json_field_base(const external_type &data) : field(data)
  {
  }

  /// \brief Moving ctor for external type.
  inline json_field_base(external_type &&data) : field(std::move(data))
  {
  }

  /// \brief Copying ctor for json field structure.
  inline json_field_base(const json_field_base &rhs) //
      noexcept(noexcept(stored_type(std::declval<const stored_type &>())))
      : field(rhs.field)
  {
  }

  /// \brief Moving ctor for json field structure.
  inline json_field_base(json_field_base &&rhs) //
      noexcept(noexcept(stored_type(std::declval<stored_type &&>())))
      : field(std::move(rhs.field))
  {
  }

  /// \brief Copying assignment operator for external type.
  inline json_field_base &operator=(const external_type &rhs)
  {
    field = rhs;
  }

  /// \brief Moving assignment operator for external type.
  inline json_field_base &operator=(external_type &&rhs)
  {
    field = std::move(rhs);
  }

  /// \brief Copying assignment operator for json field structure.
  inline json_field_base &operator=(const json_field_base &rhs)
  {
    field = rhs.field;
  }

  /// \brief Moving assignment operator for json field structure.
  inline json_field_base &operator=(json_field_base &&rhs)
  {
    field = std::move(rhs.field);
  }

  /* Required zone */

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && !is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data) : field(data[field_name])                              //
  {
  }

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && !is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data) : field(data[field_name])                                   //
  {
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && !is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data)                                                   //
  {
    const auto &array = data[field_name];
    auto data_begin = array.begin();
    auto data_end = array.end();
    field.reserve(data_end - data_begin);
    for (auto iter = data_begin; iter != data_end; ++iter)
      field.push_back(std::move(clean_type(*iter)));
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && !is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data)                                                        //
  {
    auto &array = data[field_name];
    auto data_begin = array.begin();
    auto data_end = array.end();
    field.reserve(data_end - data_begin);
    for (auto iter = data_begin; iter != data_end; ++iter)
      field.push_back(std::move(clean_type(std::move(*iter))));
  }

  /* Optional zone */

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data)                                                       //
  {
    if (data.contains(field_name))
      field = data[field_name];
  }

  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data)                                                            //
  {
    if (data.contains(field_name))
      field = std::move(data[field_name]);
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data)                                                  //
  {
    if (data.contains(field_name))
    {
      const auto &array = data[field_name];
      auto data_begin = array.begin();
      auto data_end = array.end();
      field->reserve(data_end - data_begin);
      for (auto iter = data_begin; iter != data_end; ++iter)
        field->push_back(std::move(clean_type(*iter)));
    }
  }

  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data)                                                       //
  {
    if (data.contains(field_name))
    {
      auto &array = data[field_name];
      auto data_begin = array.begin();
      auto data_end = array.end();
      field->reserve(data_end - data_begin);
      for (auto iter = data_begin; iter != data_end; ++iter)
        field->push_back(std::move(clean_type(std::move(*iter))));
    }
  }

  /* Usability zone */

  /// \brief Conversion operator for not optional and not array stored data types.
  [[nodiscard]] constexpr inline operator external_type() const
  {
    if constexpr (is_optional)
      field.value();
    else
      return field;
  }

  /// \brief Conversion operator for not optional and not array stored data types.
  [[nodiscard]] constexpr inline operator external_type &()
  {
    if constexpr (is_optional)
      return field.value();
    else
      return field;
  }

  /// \brief Conversion operator for not optional and not array stored data types.
  [[nodiscard]] constexpr inline operator const external_type &() const
  {
    if constexpr (is_optional)
      field.value();
    else
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
using json_field_required = json_field_base<T, name, true>;

template <typename T, auto &name> //
using json_field_optional = json_field_base<T, name, false>;

} // namespace json
