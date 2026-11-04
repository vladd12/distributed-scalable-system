#include <gtest/gtest.h>
#include <http/common.hpp>
#include <sstream>

TEST(http_headers_test, parse_test_01)
{
  static const std::string data = //
      "User-Agent: curl\r\n"
      "Host: localhost\r\n"
      "Accept-Language: en, mi\r\n";
  std::istringstream stream(data);
  const http::headers_t headers = http::headers_t::parse(stream);

  ASSERT_EQ(headers.size(), 3);
  auto search = headers.find("user-agent");
  ASSERT_NE(search, headers.cend());
  ASSERT_EQ(search->second, "curl");

  search = headers.find("host");
  ASSERT_NE(search, headers.cend());
  ASSERT_EQ(search->second, "localhost");

  search = headers.find("accept-language");
  ASSERT_NE(search, headers.cend());
  ASSERT_EQ(search->second, "en, mi");

  search = headers.find("content-length");
  ASSERT_EQ(search, headers.cend());
}

TEST(http_headers_test, parse_test_02)
{
  static const std::string data = //
      "\r\n   Test   :      Header  \r\n\r\n";

  std::istringstream stream(data);
  const http::headers_t headers = http::headers_t::parse(stream);

  ASSERT_EQ(headers.size(), 0);
}
