#pragma once

#include <config/json/fixed_string.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace detail
{

/// \brief Just little helper, that hold some types.
template <typename... Ts> //
struct type_holder
{
};

} // namespace detail

namespace json
{
using njson = nlohmann::json;

template <typename T, utils::fixed_string name, bool required> //
class json_field_base
{
public:
  // Static type instance restrictions
  static_assert(!std::is_const_v<T>, "Must be not a const type");
  static_assert(!std::is_reference_v<T>, "Must be not a reference type");
  static_assert(!std::is_rvalue_reference_v<T>, "Must be not a rvalue reference type");

  static constexpr std::string_view field_name = name.view(); ///< json field name
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

  /// \brief external_type is type for external acces to stored field data
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

  /// \brief Copying ctor for not optional and not array stored type.
  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && !is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data) : field(data[field_name])                              //
  {
  }

  /// \brief Moving ctor for not optional and not array stored type.
  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && !is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data) : field(data[field_name])                                   //
  {
  }

  /// \brief Copying ctor for stored not optional array type.
  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && !is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data)                                                   //
  {
    const auto &array = data[field_name];
    field.reserve(array.size());
    std::transform(array.begin(), array.end(), std::back_inserter(field), //
        [](const auto &elem) { return clean_type(elem); });
  }

  /// \brief Moving ctor for stored not optional array type.
  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && !is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data)                                                        //
  {
    auto &array = data[field_name];
    field.reserve(array.size());
    std::transform(array.begin(), array.end(), std::back_inserter(field), //
        [](auto &&elem) { return clean_type(std::move(elem)); });
  }

  /* Optional zone */

  /// \brief Copying ctor for stored not array optional type.
  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data)                                                       //
  {
    if (data.contains(field_name))
      field.emplace(data[field_name]);
  }

  /// \brief Moving ctor for stored not array optional type.
  template <typename Element = T, std::enable_if_t<!std::is_array_v<Element> && is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data)                                                            //
  {
    if (data.contains(field_name))
      field.emplace(std::move(data[field_name]));
  }

  /// \brief Copying ctor for stored optional array type.
  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && is_optional, bool> = true> //
  inline explicit json_field_base(const njson &data)                                                  //
  {
    if (data.contains(field_name))
    {
      const auto &array = data[field_name];
      std::vector<clean_type> result;
      result.reserve(array.size());
      std::transform(array.begin(), array.end(), std::back_inserter(result), //
          [](const auto &elem) { return clean_type(elem); });
      field.emplace(std::move(result));
    }
  }

  /// \brief Moving ctor for stored optional array type.
  template <typename Array = T, std::enable_if_t<std::is_array_v<Array> && is_optional, bool> = true> //
  inline explicit json_field_base(njson &&data)                                                       //
  {
    if (data.contains(field_name))
    {
      auto &array = data[field_name];
      std::vector<clean_type> result;
      result.reserve(array.size());
      std::transform(array.begin(), array.end(), std::back_inserter(result), //
          [](auto &&elem) { return clean_type(std::move(elem)); });
      field.emplace(std::move(result));
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

  /// \brief Comparison operator for equality checking.
  [[nodiscard]] inline bool operator==(const external_type &rhs) const noexcept
  {
    if constexpr (is_optional)
    {
      if (field.has_value())
        return field.value() == rhs;
      else
        return false;
    }
    else
      return field == rhs;
  }

  /// \brief Comparison operator for equality checking.
  [[nodiscard]] inline bool operator==(const json_field_base &rhs) const noexcept
  {
    return field == rhs.field;
  }

  /// \brief Comparison operator for inequality checking.
  [[nodiscard]] inline bool operator!=(const external_type &rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /// \brief Comparison operator for inequality checking.
  [[nodiscard]] inline bool operator!=(const json_field_base &rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /// \brief Checks whether field contains a value.
  [[nodiscard]] constexpr inline bool has_value() const noexcept
  {
    if constexpr (is_optional)
      return field.has_value();
    else
      return true;
  }

  /// \brief Useful operator-> for cases when we store a struct type or a class type.
  template <typename R = T, std::enable_if_t<std::is_class_v<R>, bool> = true> //
  [[nodiscard]] constexpr inline R *operator->() noexcept
  {
    if constexpr (is_optional)
      return field.operator->();
    else
      return &field;
  }

  /// \brief Useful operator-> for cases when we store a struct type or a class type. Const overload.
  template <typename R = T, std::enable_if_t<std::is_class_v<R>, bool> = true> //
  [[nodiscard]] constexpr inline const R *operator->() const noexcept
  {
    if constexpr (is_optional)
      return field.operator->();
    else
      return &field;
  }

  /// \brief Returns size of stored vector for cases when stored type is array.
  template <typename Size = std::enable_if_t<is_array, std::size_t>> //
  [[nodiscard]] inline Size size() const noexcept
  {
    if constexpr (is_optional)
    {
      if (field.has_value())
        return field.value().size();
      else
        return 0;
    }
    else
      return field.size();
  }

  /// \brief Returns reference to to the element at specified location.
  /// \details Can throw std::bad_optional_access and std::out_of_range exceptions.
  template <typename R = std::enable_if_t<is_array, clean_type>> //
  [[nodiscard]] inline R &operator[](const std::size_t pos)
  {
    if constexpr (is_optional)
      return field.value().at(pos);
    else
      return field.at(pos);
  }

  /// \brief Returns const reference to to the element at specified location.
  /// \details Can throw std::bad_optional_access and std::out_of_range exceptions.
  template <typename R = std::enable_if_t<is_array, clean_type>> //
  [[nodiscard]] inline const R &operator[](const std::size_t pos) const
  {
    if constexpr (is_optional)
      return field.value().at(pos);
    else
      return field.at(pos);
  }
};

template <typename T, utils::fixed_string name> //
using json_field_required = json_field_base<T, name, true>;

template <typename T, utils::fixed_string name> //
using json_field_optional = json_field_base<T, name, false>;

} // namespace json
