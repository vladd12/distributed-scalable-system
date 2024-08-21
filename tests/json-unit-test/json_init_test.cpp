#include <config/json/json_struct.hpp>
#include <config/json/json_unnamed_struct.hpp>
#include <gtest/gtest.h>

namespace base_test
{
using namespace detail;
using namespace json;

constexpr static auto base_full = R"(
{
  "value-int": 148536,
  "value-bool": true,
  "value-string": "some string, dude"
}
)";

constexpr static auto base_optional = R"(
{
  "value-int": 258564
}
)";

constexpr static auto value_int_str = "value-int";
constexpr static auto value_bool_str = "value-bool";
constexpr static auto value_string_str = "value-string";

struct root_tag
{
  json_field_required<int, value_int_str> value_int;
  json_field_optional<bool, value_bool_str> value_bool;
  json_field_optional<std::string, value_string_str> value_string;
  static constexpr auto holder = detail::type_holder< //
      decltype(value_int), decltype(value_bool), decltype(value_string)> {};
};
typedef json_unnamed_struct<root_tag> root_configuration;

TEST(json_unit_test, base_test_full)
{
  njson json = njson::parse(base_full);
  root_configuration cfg { json };
  static_assert(cfg.value_int.has_value());
  ASSERT_EQ(cfg.value_int, 148536);
  ASSERT_TRUE(cfg.value_bool.has_value());
  ASSERT_EQ(cfg.value_bool, true);
  ASSERT_TRUE(cfg.value_string.has_value());
  ASSERT_EQ(cfg.value_string, "some string, dude");
  const decltype(cfg.value_int) int_copy { cfg.value_int };
  const decltype(cfg.value_int) int_move_copy { std::move(cfg.value_int) };
  ASSERT_EQ(int_copy, int_move_copy);
  const decltype(cfg.value_string) str_copy { cfg.value_string };
  const decltype(cfg.value_string) str_move_copy { std::move(cfg.value_string) };
  ASSERT_EQ(str_copy, str_move_copy);
}

TEST(json_unit_test, base_test_optional)
{
  root_configuration cfg { std::move(njson::parse(base_optional)) };
  ASSERT_EQ(cfg.value_int, 258564);
  ASSERT_FALSE(cfg.value_bool.has_value());
  ASSERT_FALSE(cfg.value_string.has_value());
  const decltype(cfg.value_bool) bool_copy { cfg.value_bool };
  const decltype(cfg.value_bool) bool_move_copy { std::move(cfg.value_bool) };
  ASSERT_EQ(bool_copy, bool_move_copy);
  const decltype(cfg.value_string) str_copy { cfg.value_string };
  const decltype(cfg.value_string) str_move_copy { std::move(cfg.value_string) };
  ASSERT_EQ(str_copy, str_move_copy);
}

constexpr static auto base_array_full = R"(
{
  "values": [ "one", "two", "three" ],
  "more-values": [ 1, 2, 3 ]
}
)";

constexpr static auto base_array_optional = R"(
{
  "more-values": []
}
)";

constexpr static auto values_str = "values";
constexpr static auto more_values_str = "more-values";

struct array_tag
{
  json_field_optional<std::string[], values_str> values;
  json_field_required<int[], more_values_str> more_values;
  static constexpr auto holder = detail::type_holder<decltype(values), decltype(more_values)> {};
};
typedef json_unnamed_struct<array_tag> array_configuration;

TEST(json_unit_test, base_array_test_full)
{
  const std::vector<std::string> values_cmp { "one", "two", "three" };
  const std::vector<int> more_values_cmp { 1, 2, 3 };
  njson json = njson::parse(base_array_full);
  array_configuration cfg { json };
  static_assert(cfg.more_values.has_value());
  ASSERT_TRUE(cfg.values.has_value());
  ASSERT_EQ(cfg.values, values_cmp);
  ASSERT_EQ(cfg.more_values, more_values_cmp);
  const decltype(cfg.values) vals_copy { cfg.values };
  const decltype(cfg.values) vals_move_copy { std::move(cfg.values) };
  ASSERT_EQ(vals_copy, vals_move_copy);
  const decltype(cfg.more_values) more_vals_copy { cfg.more_values };
  const decltype(cfg.more_values) more_vals_move_copy { std::move(cfg.more_values) };
  ASSERT_EQ(more_vals_copy, more_vals_move_copy);
}

TEST(json_unit_test, base_array_test_optional)
{
  array_configuration cfg { std::move(njson::parse(base_array_optional)) };
  static_assert(cfg.more_values.has_value());
  ASSERT_FALSE(cfg.values.has_value());
  ASSERT_EQ(cfg.more_values, std::vector<int> {});
  const decltype(cfg.values) vals_copy { cfg.values };
  const decltype(cfg.values) vals_move_copy { std::move(cfg.values) };
  ASSERT_EQ(vals_copy, vals_move_copy);
  const decltype(cfg.more_values) more_vals_copy { cfg.more_values };
  const decltype(cfg.more_values) more_vals_move_copy { std::move(cfg.more_values) };
  ASSERT_EQ(more_vals_copy, more_vals_move_copy);
}

} // namespace base_test

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

constexpr static auto full_logger = R"(
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
typedef json_unnamed_struct<rotate_tag> rotate_configuration;

struct async_tag
{
  json_field_required<int, thread_pool_size_str> thread_pool_size;
  json_field_required<int, queue_size_str> queue_size;
  static constexpr auto holder = detail::type_holder<decltype(thread_pool_size), decltype(queue_size)> {};
};
typedef json_unnamed_struct<async_tag> async_configuration;

struct logger_tag
{
  json_field_required<std::string, name_str> name;
  json_field_required<std::string, filepath_str> filepath;
  json_field_optional<std::string, pattern_str> pattern;
  json_field_optional<rotate_configuration, rotate_str> rotate;
  json_field_optional<async_configuration, async_str> async;
  static constexpr auto holder = detail::type_holder<                                          //
      decltype(name), decltype(filepath), decltype(pattern), decltype(rotate), decltype(async) //
      > {};                                                                                    //
};
typedef json_unnamed_struct<logger_tag> logger_configuration;

struct root_tag
{
  json_field_required<logger_configuration, logger_str> logger;
  static constexpr auto holder = detail::type_holder<decltype(logger)> {};
};
typedef json_unnamed_struct<root_tag> root_configuration;

TEST(json_unit_test, logger_test_copy)
{
  njson json = njson::parse(full_logger);
  root_configuration cfg { json };
  ASSERT_EQ(cfg.logger->name, "default_logger");
  ASSERT_EQ(cfg.logger->filepath, "logs/log.txt");
  ASSERT_EQ(cfg.logger->pattern, "[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  ASSERT_EQ(cfg.logger->rotate->max_size, 10);
  ASSERT_EQ(cfg.logger->rotate->max_files, 10);
  ASSERT_EQ(cfg.logger->async->thread_pool_size, 1);
  ASSERT_EQ(cfg.logger->async->queue_size, 1024);
}

TEST(json_unit_test, logger_test_move)
{
  njson json = njson::parse(full_logger);
  root_configuration cfg { std::move(json) };
  ASSERT_EQ(cfg.logger->name, "default_logger");
  ASSERT_EQ(cfg.logger->filepath, "logs/log.txt");
  ASSERT_EQ(cfg.logger->pattern, "[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  ASSERT_EQ(cfg.logger->rotate->max_size, 10);
  ASSERT_EQ(cfg.logger->rotate->max_files, 10);
  ASSERT_EQ(cfg.logger->async->thread_pool_size, 1);
  ASSERT_EQ(cfg.logger->async->queue_size, 1024);
}

} // namespace logger_test

namespace array_test
{
using namespace detail;
using namespace json;

constexpr static auto array_complex_json = R"(
{
  "name": "default_logger",
  "value": "some_value",
  "flag": true,
  "options": [
    { "key": "AAA", "value": "111", "optionality": [1, 2, 3] },
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
constexpr static auto optionality_str = "optionality";
constexpr static auto flag_str = "flag";
constexpr static auto options_str = "options";
constexpr static auto other_str = "other";
constexpr static auto AAA_str = "AAA";
constexpr static auto BBB_str = "BBB";

struct key_value_tag
{
  json_field_required<std::string, key_str> key;
  json_field_required<std::string, value_str> value;
  json_field_optional<int[], optionality_str> optionality;
  static constexpr auto holder = detail::type_holder<decltype(key), decltype(value), decltype(optionality)> {};
};
typedef json_unnamed_struct<key_value_tag> key_value_configuration;

struct other_tag
{
  json_field_required<int, AAA_str> AAA;
  json_field_required<int, BBB_str> BBB;
  static constexpr auto holder = detail::type_holder<decltype(AAA), decltype(BBB)> {};
};
typedef json_unnamed_struct<other_tag> other_configuration;

struct root_tag
{
  json_field_required<std::string, name_str> name;
  json_field_required<std::string, value_str> value;
  json_field_optional<bool, flag_str> flag;
  json_field_required<key_value_configuration[], options_str> options;
  json_field_optional<other_configuration, other_str> other;
  static constexpr auto holder = detail::type_holder< //
      decltype(name), decltype(value), decltype(flag), decltype(options), decltype(other)> {};
};
typedef json_unnamed_struct<root_tag> root_configuration;

TEST(json_unit_test, array_complex_test)
{
  njson json = njson::parse(array_complex_json);
  root_configuration cfg { json };
  static_assert(cfg.name.has_value());
  static_assert(cfg.value.has_value());
  static_assert(cfg.options.has_value());
  ASSERT_EQ(cfg.name, "default_logger");
  ASSERT_EQ(cfg.value, "some_value");
  ASSERT_TRUE(cfg.flag.has_value());
  ASSERT_EQ(cfg.flag, true);
  ASSERT_TRUE(cfg.other.has_value());
  ASSERT_EQ(cfg.other->AAA, 111);
  ASSERT_EQ(cfg.other->BBB, 222);
  ASSERT_EQ(cfg.options.size(), 2);
  const auto &elem0 = cfg.options[0];
  ASSERT_EQ(elem0.key, "AAA");
  ASSERT_EQ(elem0.value, "111");
  ASSERT_TRUE(elem0.optionality.has_value());
  ASSERT_EQ(elem0.optionality.size(), 3);
  ASSERT_EQ(elem0.optionality, (std::vector<int> { 1, 2, 3 }));
  auto &elem1 = cfg.options[1];
  ASSERT_EQ(elem1.key, "BBB");
  ASSERT_EQ(elem1.value, "222");
  ASSERT_FALSE(elem1.optionality.has_value());
  ASSERT_EQ(elem1.optionality.size(), 0);
}

} // namespace array_test
