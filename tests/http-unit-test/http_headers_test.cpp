#include <core/errors.hpp>
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

  ASSERT_EQ(headers.size(), 0ull);
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
  ASSERT_EQ(h.size(), 1ull);
  ASSERT_EQ(h.find("content-type")->second, "text/html");
}

TEST(http_headers_test, values_are_trimmed)
{
  std::istringstream s("X-Test:   spaced value   \r\n\r\n");
  auto h = http::headers_t::parse(s);
  ASSERT_EQ(h.find("X-Test")->second, "spaced value");
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

// ============================================================================
// Duplicate security-sensitive headers
// ============================================================================

TEST(http_headers_test, duplicate_content_length_same_value_throws)
{
  std::istringstream s("Content-Length: 5\r\nContent-Length: 5\r\n\r\n");
  EXPECT_THROW((void)http::headers_t::parse(s), core::http_error);
}

TEST(http_headers_test, duplicate_content_length_different_values_throws)
{
  std::istringstream s("Content-Length: 5\r\nContent-Length: 10\r\n\r\n");
  EXPECT_THROW((void)http::headers_t::parse(s), core::http_error);
}

TEST(http_headers_test, duplicate_content_length_case_insensitive)
{
  // Different casing — still duplicate after normalization.
  std::istringstream s("Content-Length: 5\r\ncontent-length: 5\r\n\r\n");
  EXPECT_THROW((void)http::headers_t::parse(s), core::http_error);
}

TEST(http_headers_test, duplicate_host_throws)
{
  std::istringstream s("Host: a.example\r\nHost: b.example\r\n\r\n");
  EXPECT_THROW((void)http::headers_t::parse(s), core::http_error);
}

TEST(http_headers_test, duplicate_host_case_insensitive)
{
  std::istringstream s("Host: a.example\r\nhost: a.example\r\n\r\n");
  EXPECT_THROW((void)http::headers_t::parse(s), core::http_error);
}

TEST(http_headers_test, single_content_length_and_host_ok)
{
  std::istringstream s("Host: localhost\r\nContent-Length: 0\r\n\r\n");
  const auto h = http::headers_t::parse(s);
  ASSERT_EQ(h.size(), 2ull);
  ASSERT_TRUE(h.contains("Host"));
  ASSERT_TRUE(h.contains("Content-Length"));
}

// ============================================================================
// Duplicate non-sensitive headers: allowed, first-wins
// ============================================================================

TEST(http_headers_test, duplicate_non_sensitive_header_first_wins)
{
  std::istringstream s("X-Foo: a\r\nX-Foo: b\r\n\r\n");
  const auto h = http::headers_t::parse(s);
  ASSERT_EQ(h.size(), 1ull);
  ASSERT_EQ(h.find("x-foo")->second, "a"); // first-wins
}

// ============================================================================
// Initializer-list path — same semantics
// ============================================================================

TEST(http_headers_test, initializer_list_duplicate_content_length_throws)
{
  EXPECT_THROW((http::headers_t { { "Content-Length", "5" }, { "Content-Length", "10" } }), core::http_error);
}

TEST(http_headers_test, initializer_list_duplicate_host_throws)
{
  EXPECT_THROW((http::headers_t { { "Host", "a" }, { "host", "b" } }), core::http_error);
}

TEST(http_headers_test, initializer_list_duplicate_non_sensitive_allowed)
{
  http::headers_t h { { "X-Foo", "a" }, { "X-Foo", "b" } };
  ASSERT_EQ(h.size(), 1ull);
  ASSERT_EQ(h.find("x-foo")->second, "a"); // first-wins
}

TEST(http_headers_test, erase_iterators)
{
  http::headers_t h { { "X-Foo", "a" }, { "X-Bar", "b" }, { "X-Custom-1", "c" }, { "X-Custom-2", "d" } };
  ASSERT_EQ(h.size(), 4ull);
  auto iter = h.erase(h.cbegin());
  ASSERT_NE(iter, h.end());
  ASSERT_EQ(h.size(), 3ull);
  iter = h.erase(h.end());
  ASSERT_EQ(iter, h.end());
  ASSERT_EQ(h.size(), 3ull);
  auto last = ++(++h.begin());
  ASSERT_EQ(h.erase(last), h.end());
  ASSERT_EQ(h.size(), 2ull);
}

TEST(http_headers_test, erase_keys)
{
  http::headers_t h { { "X-Foo", "a" }, { "X-Bar", "b" } };
  ASSERT_EQ(h.size(), 2ull);
  ASSERT_TRUE(h.erase("X-Foo"));
  ASSERT_EQ(h.size(), 1ull);
  ASSERT_FALSE(h.erase("X-Test"));
  ASSERT_EQ(h.size(), 1ull);
}
