#pragma once

#include "config/configuration_v2.hpp"

namespace config
{

void foo(const detail::njson &data)
{
  constexpr static auto foo_name = "foo";
  typedef detail::json_field_required<int, foo_name> foo_int;
  typedef detail::json_field_optional<int, foo_name> foo_int_opt;
  static_assert(sizeof(foo_int) == sizeof(int));
  foo_int inst1 { data };
  int some1 = inst1;
  foo_int inst2 { std::move(inst1) };
  int some2 = inst2;
  assert(some1 == some2);
  foo_int_opt opt1 { int(inst2) };
  foo_int_opt opt2 { data };
}

} // namespace config
