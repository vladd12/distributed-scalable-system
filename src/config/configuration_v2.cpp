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

namespace test
{
using namespace detail;

constexpr static auto logger_str = "logger";
constexpr static auto name_str = "name";
constexpr static auto filepath_str = "filepath";
constexpr static auto rotate_str = "rotate";
constexpr static auto max_size_str = "max-size";
constexpr static auto max_files_str = "max-files";
constexpr static auto async_str = "async";
constexpr static auto thread_pool_size_str = "thread-pool-size";
constexpr static auto queue_size_str = "queue-size";

// struct rotate_configuration : public json_struct<rotate_configuration, rotate_str>
//{
//};

void test01()
{
  const njson ex1 = njson::parse(R"(
{
    "logger": {
        "name": "default_logger",
        "filepath": "logs/log.txt",
        "pattern": "[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v",
        "rotate": {
            "max-size": 10,
            "max-files" : 10
        },
        "async": {
            "thread-pool-size": 1,
            "queue-size": 1024
        }
    },
}
)");
}

} // namespace test
