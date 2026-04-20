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
  ASSERT_EQ(http::status_text_for(200), "OK");
  ASSERT_EQ(http::status_text_for(201), "Created");
  ASSERT_EQ(http::status_text_for(204), "No Content");
  ASSERT_EQ(http::status_text_for(301), "Moved Permanently");
  ASSERT_EQ(http::status_text_for(302), "Found");
  ASSERT_EQ(http::status_text_for(304), "Not Modified");
  ASSERT_EQ(http::status_text_for(400), "Bad Request");
  ASSERT_EQ(http::status_text_for(401), "Unauthorized");
  ASSERT_EQ(http::status_text_for(403), "Forbidden");
  ASSERT_EQ(http::status_text_for(404), "Not Found");
  ASSERT_EQ(http::status_text_for(405), "Method Not Allowed");
  ASSERT_EQ(http::status_text_for(408), "Request Timeout");
  ASSERT_EQ(http::status_text_for(413), "Payload Too Large");
  ASSERT_EQ(http::status_text_for(431), "Request Header Fields Too Large");
  ASSERT_EQ(http::status_text_for(500), "Internal Server Error");
  ASSERT_EQ(http::status_text_for(502), "Bad Gateway");
  ASSERT_EQ(http::status_text_for(503), "Service Unavailable");
  ASSERT_EQ(http::status_text_for(600), "Unknown");
  ASSERT_EQ(http::status_text_for(700), "Unknown");
}
