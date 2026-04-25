#include <core/conversion.hpp>
#include <gtest/gtest.h>
#include <http/common.hpp>

TEST(http_common_test, string_to_method_test)
{
  ASSERT_EQ(http::string_to_method("GET"), http::method::GET);
  ASSERT_EQ(http::string_to_method("POST"), http::method::POST);
  ASSERT_EQ(http::string_to_method("PUT"), http::method::PUT);
  ASSERT_EQ(http::string_to_method("DELETE"), http::method::DELETE_);
  ASSERT_EQ(http::string_to_method("PATCH"), http::method::PATCH);
  ASSERT_EQ(http::string_to_method("HEAD"), http::method::HEAD);
  ASSERT_EQ(http::string_to_method("OPTIONS"), http::method::OPTIONS);
  ASSERT_EQ(http::string_to_method("TEST"), http::method::UNKNOWN);
}

TEST(http_common_test, method_to_string_test)
{
  ASSERT_EQ(http::method_to_string(http::method::GET), "GET");
  ASSERT_EQ(http::method_to_string(http::method::POST), "POST");
  ASSERT_EQ(http::method_to_string(http::method::PUT), "PUT");
  ASSERT_EQ(http::method_to_string(http::method::DELETE_), "DELETE");
  ASSERT_EQ(http::method_to_string(http::method::PATCH), "PATCH");
  ASSERT_EQ(http::method_to_string(http::method::HEAD), "HEAD");
  ASSERT_EQ(http::method_to_string(http::method::OPTIONS), "OPTIONS");
  ASSERT_EQ(http::method_to_string(http::method::UNKNOWN), "UNKNOWN");

  http::method value = http::method::GET;
  constexpr auto integer_value = 0xffff;
  static_assert(sizeof(value) == sizeof(integer_value));
  std::memcpy(&value, &integer_value, sizeof(value));
  ASSERT_EQ(http::method_to_string(value), "UNKNOWN");
}

TEST(http_common_test, status_text_for_test)
{
  ASSERT_EQ(http::status_text_for(http::status_code::OK), "OK");
  ASSERT_EQ(http::status_text_for(http::status_code::CREATED), "Created");
  ASSERT_EQ(http::status_text_for(http::status_code::NO_CONTENT), "No Content");
  ASSERT_EQ(http::status_text_for(http::status_code::MOVED_PERMANENTLY), "Moved Permanently");
  ASSERT_EQ(http::status_text_for(http::status_code::FOUND), "Found");
  ASSERT_EQ(http::status_text_for(http::status_code::NOT_MODIFIED), "Not Modified");
  ASSERT_EQ(http::status_text_for(http::status_code::BAD_REQUEST), "Bad Request");
  ASSERT_EQ(http::status_text_for(http::status_code::UNAUTHORIZED), "Unauthorized");
  ASSERT_EQ(http::status_text_for(http::status_code::FORBIDDEN), "Forbidden");
  ASSERT_EQ(http::status_text_for(http::status_code::NOT_FOUND), "Not Found");
  ASSERT_EQ(http::status_text_for(http::status_code::NOT_ALLOWED), "Method Not Allowed");
  ASSERT_EQ(http::status_text_for(http::status_code::REQUEST_TIMEOUT), "Request Timeout");
  ASSERT_EQ(http::status_text_for(http::status_code::LARGE_PAYLOAD), "Payload Too Large");
  ASSERT_EQ(http::status_text_for(http::status_code::LARGE_HEADERS), "Request Header Fields Too Large");
  ASSERT_EQ(http::status_text_for(http::status_code::INTERNAL_SERVER_ERROR), "Internal Server Error");
  ASSERT_EQ(http::status_text_for(http::status_code::BAD_GATEWAY), "Bad Gateway");
  ASSERT_EQ(http::status_text_for(http::status_code::UNAVAILABLE), "Service Unavailable");
  ASSERT_EQ(http::status_text_for(core::from_underlying<http::status_code>(std::uint16_t(600))), "Unknown");
  ASSERT_EQ(http::status_text_for(core::from_underlying<http::status_code>(std::uint16_t(700))), "Unknown");
}
