#include <gtest/gtest.h>
#include <http/headers.hpp>
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

TEST(http_headers_test, case_insensitive_lookup)
{
  http::headers_t h;
  h["Host"] = "localhost";
  ASSERT_TRUE(h.contains("host"));
  ASSERT_TRUE(h.contains("HOST"));
  ASSERT_TRUE(h.contains("Host"));
  ASSERT_EQ(h.find("HOST")->second, "localhost");
}

TEST(http_headers_test, operator_brackets_normalizes)
{
  http::headers_t h;
  h["Content-Type"] = "text/plain";
  h["CONTENT-TYPE"] = "text/html"; // same key after normalization
  ASSERT_EQ(h.size(), 1u);
  ASSERT_EQ(h.find("content-type")->second, "text/html");
}

TEST(http_headers_test, values_are_trimmed)
{
  std::istringstream s("X-Test:   spaced value   \r\n\r\n");
  auto h = http::headers_t::parse(s);
  ASSERT_EQ(h.find("x-test")->second, "spaced value");
}

TEST(http_headers_test, iterator_reaches_end)
{
  http::headers_t h;
  h["A"] = "1";
  h["B"] = "2";
  int count = 0;
  for (auto it = h.begin(); it != h.end(); ++it)
    ++count;
  ASSERT_EQ(count, 2);
}
