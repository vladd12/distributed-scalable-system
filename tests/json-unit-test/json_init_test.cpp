#include <config/json/json_struct.hpp>
#include <config/json/json_unnamed_struct.hpp>
#include <gtest/gtest.h>

namespace logger_test
{
using namespace detail;
using namespace json;

constexpr static auto logger_str = "logger";
constexpr static auto name_str = "name";
constexpr static auto filepath_str = "filepath";
constexpr static auto pattern_str = "pattern";
constexpr static auto rotate_str = "rotate";
constexpr static auto max_size_str = "max-size";
constexpr static auto max_files_str = "max-files";
constexpr static auto async_str = "async";
constexpr static auto thread_pool_size_str = "thread-pool-size";
constexpr static auto queue_size_str = "queue-size";

constexpr static auto data01 = R"(
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
    }
}
)";

struct rotate_tag
{
  json_field_required<int, max_size_str> max_size;
  json_field_required<int, max_files_str> max_files;
  static constexpr auto holder = detail::type_holder<decltype(max_size), decltype(max_files)> {};
};
typedef json_struct<rotate_tag, rotate_str> rotate_configuration;

struct async_tag
{
  json_field_required<int, thread_pool_size_str> thread_pool_size;
  json_field_required<int, queue_size_str> queue_size;
  static constexpr auto holder = detail::type_holder<decltype(thread_pool_size), decltype(queue_size)> {};
};
typedef json_struct<async_tag, async_str> async_configuration;

struct logger_tag
{
  json_field_required<std::string, name_str> name;
  json_field_required<std::string, filepath_str> filepath;
  json_field_optional<std::string, pattern_str> pattern;
  rotate_configuration rotate;
  async_configuration async;
  static constexpr auto holder = detail::type_holder<                                          //
      decltype(name), decltype(filepath), decltype(pattern), decltype(rotate), decltype(async) //
      > {};                                                                                    //
};
typedef json_struct<logger_tag, logger_str> logger_configuration;

struct root_tag
{
  logger_configuration logger;
  static constexpr auto holder = detail::type_holder<decltype(logger)> {};
};
typedef json_unnamed_struct<root_tag> root_configuration;

} // namespace logger_test

TEST(json_unit_test, logger_test_copy)
{
  using namespace logger_test;
  njson json = njson::parse(data01);
  root_configuration cfg { json };
  // TODO: binary operator== overload for comparing data
  const std::string &name = cfg.logger.name;
  ASSERT_EQ(name, "default_logger");
  const std::string &filepath = cfg.logger.filepath;
  ASSERT_EQ(filepath, "logs/log.txt");
  const std::string &pattern = cfg.logger.pattern;
  ASSERT_EQ(pattern, "[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  const int &max_size = cfg.logger.rotate.max_size;
  ASSERT_EQ(max_size, 10);
  const int &max_files = cfg.logger.rotate.max_files;
  ASSERT_EQ(max_files, 10);
  const int &thread_pool_size = cfg.logger.async.thread_pool_size;
  ASSERT_EQ(thread_pool_size, 1);
  const int &queue_size = cfg.logger.async.queue_size;
  ASSERT_EQ(queue_size, 1024);
}

TEST(json_unit_test, logger_test_move)
{
  using namespace logger_test;
  njson json = njson::parse(data01);
  root_configuration cfg { std::move(json) };
  const std::string &name = cfg.logger.name;
  ASSERT_EQ(name, "default_logger");
  const std::string &filepath = cfg.logger.filepath;
  ASSERT_EQ(filepath, "logs/log.txt");
  const int &max_size = cfg.logger.rotate.max_size;
  ASSERT_EQ(max_size, 10);
  const int &max_files = cfg.logger.rotate.max_files;
  ASSERT_EQ(max_files, 10);
  const int &thread_pool_size = cfg.logger.async.thread_pool_size;
  ASSERT_EQ(thread_pool_size, 1);
  const int &queue_size = cfg.logger.async.queue_size;
  ASSERT_EQ(queue_size, 1024);
}

namespace array_required_test
{
using namespace detail;
using namespace json;

constexpr static auto data02 = R"(
{
  "name": "default_logger",
  "value": "some_value",
  "flag": true,
  "options": [
    { "key": "AAA", "value": "111" },
    { "key": "BBB", "value": "222" }
  ],
  "other": {
    "AAA": 111,
    "BBB": 222
  }
}
)";

constexpr static auto name_str = "name";
constexpr static auto key_str = "key";
constexpr static auto value_str = "value";
constexpr static auto flag_str = "flag";
constexpr static auto options_str = "options";
constexpr static auto other_str = "other";
constexpr static auto AAA_str = "AAA";
constexpr static auto BBB_str = "BBB";

struct key_value_tag
{
  json_field_required<std::string, key_str> key;
  json_field_required<std::string, value_str> value;
  static constexpr auto holder = detail::type_holder<decltype(key), decltype(value)> {};
};
typedef json_unnamed_struct<key_value_tag> key_value_configuration;

struct other_tag
{
  json_field_required<int, AAA_str> AAA;
  json_field_required<int, BBB_str> BBB;
  static constexpr auto holder = detail::type_holder<decltype(AAA), decltype(BBB)> {};
};
typedef json_struct<other_tag, other_str> other_configuration;

struct root_tag
{
  json_field_required<std::string, name_str> name;
  json_field_required<std::string, value_str> value;
  json_field_optional<bool, flag_str> flag;
  json_field_required<key_value_configuration[], options_str> options;
  other_configuration other;
  static constexpr auto holder = detail::type_holder< //
      decltype(name), decltype(value), decltype(flag), decltype(options), decltype(other)> {};
};
typedef json_unnamed_struct<root_tag> root_configuration;

TEST(json_unit_test, array_required_test)
{
  using namespace array_required_test;
  [[maybe_unused]] njson json = njson::parse(data02);
  // TODO: not compile this
  // root_configuration cfg { json };
}

}
