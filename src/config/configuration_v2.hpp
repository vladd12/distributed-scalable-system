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

template <typename... Ts> //
struct type_holder
{
};

// explicit deduction guide
template <typename... Ts> type_holder(Ts...) -> type_holder<Ts...>;

template <typename T, auto &name, bool required> //
class json_field_base
{
public:
  static constexpr std::string_view field_name = std::string_view(name);
  typedef typename required_datatype_selector<T, required>::type value_type;

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

  json_field_base &operator=(const json_field_base &rhs)
  {
    field = rhs.field;
  }

  json_field_base &operator=(json_field_base &&rhs)
  {
    field = std::move(rhs.field);
  }

  [[nodiscard]] operator value_type() const noexcept
  {
    return field;
  }

  [[nodiscard]] operator value_type &() noexcept
  {
    return field;
  }

  [[nodiscard]] operator const value_type &() const noexcept
  {
    return field;
  }
};

template <typename T, auto &name> //
struct json_field_required : public json_field_base<T, name, true>
{
public:
  typedef json_field_base<T, name, true> base_type;
  static constexpr std::string_view field_name = base_type::field_name;

  inline explicit json_field_required(const njson &data) : base_type(data[field_name])
  {
  }

  inline explicit json_field_required(njson &&data) : base_type(std::move(data[field_name]))
  {
  }
};

template <typename T, auto &name> //
struct json_field_optional : public json_field_base<T, name, false>
{
public:
  typedef json_field_base<T, name, true> base_type;
  static constexpr std::string_view field_name = base_type::field_name;

  inline explicit json_field_optional(const njson &data)
  {
    if (data.contains(field_name))
      this->field = data[field_name];
  }

  inline explicit json_field_optional(njson &&data)
  {
    if (data.contains(field_name))
      this->field = std::move(data[field_name]);
  }
};

template <typename T, auto &name> //
struct json_struct : public T
{
public:
  static constexpr std::string_view struct_name = std::string_view(name);

  template <typename... Ts>                                                           //
  inline explicit json_struct(const njson &data, [[maybe_unused]] type_holder<Ts...>) //
      : T(std::move(Ts(data[struct_name])...))                                        //
  {
  }

  template <typename... Ts>                                                      //
  inline explicit json_struct(njson &&data, [[maybe_unused]] type_holder<Ts...>) //
      : T(std::move(Ts(std::move(data[struct_name]))...))                        //
  {
  }
};

template <typename T> //
struct json_struct_root : public T
{
public:
  template <typename... Ts>                                                                //
  inline explicit json_struct_root(const njson &data, [[maybe_unused]] type_holder<Ts...>) //
      : T(std::move(Ts(data)...))                                                          //
  {
  }

  template <typename... Ts>                                                           //
  inline explicit json_struct_root(njson &&data, [[maybe_unused]] type_holder<Ts...>) //
      : T(std::move(Ts(std::move(data))...))                                          //
  {
  }
};

} // namespace detail

namespace config
{

void foo(const detail::njson &data);

} // namespace config
