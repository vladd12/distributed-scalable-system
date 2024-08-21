#pragma once

#include <config/json/json_field_base.hpp>

namespace json
{

template <typename T> //
struct json_struct : public T
{
public:
  template <typename... Ts>                        //
  inline explicit json_struct(const njson &data,   //
      [[maybe_unused]] detail::type_holder<Ts...>) //
      : T(std::forward<Ts>(Ts(data))...)           //
  {
  }

  template <typename... Ts>                         //
  inline explicit json_struct(njson &&data,         //
      [[maybe_unused]] detail::type_holder<Ts...>)  //
      : T(std::forward<Ts>(Ts(std::move(data)))...) //
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

} // namespace json
