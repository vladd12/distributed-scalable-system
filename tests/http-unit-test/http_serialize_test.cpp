#include <core/errors.hpp>
#include <gtest/gtest.h>
#include <http/request.hpp>
#include <http/response.hpp>
#include <utils/strings.hpp>

// ============================================================================
// response::serialize
// ============================================================================

inline const std::string cont_length_str = utils::to_lower("Content-Length");

TEST(http_serialize_test, response_200_empty_body_has_content_length_zero)
{
  auto resp = http::response::simple(http::status_code::OK);
  const auto wire = resp.serialize();
  EXPECT_NE(wire.find(cont_length_str + ": 0\r\n"), std::string::npos) << wire;
}

TEST(http_serialize_test, response_200_with_body_has_content_length)
{
  auto resp = http::response::text(http::status_code::OK, "hello");
  const auto wire = resp.serialize();
  EXPECT_NE(wire.find(cont_length_str + ": 5\r\n"), std::string::npos) << wire;
}

TEST(http_serialize_test, response_204_has_no_content_length)
{
  auto resp = http::response::simple(http::status_code::NO_CONTENT);
  const auto wire = resp.serialize();
  EXPECT_EQ(wire.find(cont_length_str), std::string::npos) << wire;
}

TEST(http_serialize_test, response_304_has_no_content_length)
{
  auto resp = http::response::simple(http::status_code::NOT_MODIFIED);
  const auto wire = resp.serialize();
  EXPECT_EQ(wire.find(cont_length_str), std::string::npos) << wire;
}

TEST(http_serialize_test, response_does_not_duplicate_explicit_content_length)
{
  auto resp = http::response::text(http::status_code::OK, "abc");
  const auto wire = resp.serialize();

  // Exactly one "Content-Length" occurrence.
  std::size_t count = 0;
  std::size_t pos = 0;
  while ((pos = wire.find(cont_length_str, pos)) != std::string::npos)
  {
    ++count;
    pos += 14;
  }
  EXPECT_EQ(count, 1u) << wire;
}

// ============================================================================
// request::serialize
// ============================================================================

TEST(http_serialize_test, request_without_host_throws)
{
  http::request req;
  req.line = { http::method::GET, "/", std::string(http::http_version) };
  std::string result;
  EXPECT_THROW(result = req.serialize(), core::http_error);
}

TEST(http_serialize_test, request_with_host_does_not_throw)
{
  http::request req;
  req.line = { http::method::GET, "/", std::string(http::http_version) };
  req.headers["Host"] = "localhost";
  std::string result;
  EXPECT_NO_THROW(result = req.serialize());
}

TEST(http_serialize_test, request_get_empty_body_no_content_length)
{
  http::request req;
  req.line = { http::method::GET, "/", std::string(http::http_version) };
  req.headers["Host"] = "localhost";
  req.headers["Connection"] = "close";
  const auto wire = req.serialize();
  EXPECT_EQ(wire.find(cont_length_str), std::string::npos) << wire;
}

TEST(http_serialize_test, request_post_empty_body_has_content_length_zero)
{
  http::request req;
  req.line = { http::method::POST, "/", std::string(http::http_version) };
  req.headers["Host"] = "localhost";
  req.headers["Connection"] = "close";
  const auto wire = req.serialize();
  EXPECT_NE(wire.find(cont_length_str + ": 0\r\n"), std::string::npos) << wire;
}

TEST(http_serialize_test, request_put_empty_body_has_content_length_zero)
{
  http::request req;
  req.line = { http::method::PUT, "/", std::string(http::http_version) };
  req.headers["Host"] = "localhost";
  const auto wire = req.serialize();
  EXPECT_NE(wire.find(cont_length_str + ": 0\r\n"), std::string::npos) << wire;
}

TEST(http_serialize_test, request_post_with_body_has_content_length)
{
  http::request req;
  req.line = { http::method::POST, "/", std::string(http::http_version) };
  req.headers["Host"] = "localhost";
  req.body = "hello";
  const auto wire = req.serialize();
  EXPECT_NE(wire.find(cont_length_str + ": 5\r\n"), std::string::npos) << wire;
}

TEST(http_serialize_test, request_does_not_duplicate_explicit_content_length)
{
  http::request req;
  req.line = { http::method::POST, "/", std::string(http::http_version) };
  req.headers["Host"] = "localhost";
  req.headers["Content-Length"] = "999"; // user explicitly set it
  req.body = "hello";

  const auto wire = req.serialize();

  std::size_t count = 0;
  std::size_t pos = 0;
  while ((pos = wire.find(cont_length_str, pos)) != std::string::npos)
  {
    ++count;
    pos += 14;
  }
  EXPECT_EQ(count, 1u) << wire;
  EXPECT_NE(wire.find(cont_length_str + ": 999\r\n"), std::string::npos) << wire;
}
