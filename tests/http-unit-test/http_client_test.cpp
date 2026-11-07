#include "http_test_helpers.hpp"

#include <core/errors.hpp>
#include <gtest/gtest.h>
#include <http/client.hpp>

using namespace http_test;

TEST(http_client_test, parses_status_and_body)
{
  const auto port = next_port();
  raw_server srv(port,
      "HTTP/1.1 201 Created\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 5\r\n"
      "Connection: close\r\n"
      "\r\n"
      "hello");

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", port);

  int status = 0;
  std::string body;
  bool done = false;

  cli.get("/", {}, [&](boost::system::error_code ec, const http::response &resp) {
    ASSERT_FALSE(ec) << ec.message();
    status = static_cast<int>(resp.line.status_code);
    body = resp.body;
    done = true;
  });
  ctx.run();

  EXPECT_TRUE(done);
  EXPECT_EQ(status, 201);
  EXPECT_EQ(body, "hello");
}

TEST(http_client_test, parses_response_headers)
{
  const auto port = next_port();
  raw_server srv(port,
      "HTTP/1.1 200 OK\r\n"
      "X-Custom: FooBar\r\n"
      "Content-Length: 2\r\n"
      "Connection: close\r\n"
      "\r\n"
      "ok");

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", port);

  bool has_custom = false;
  std::string custom_value;

  cli.get("/", {}, [&](boost::system::error_code ec, const http::response &resp) {
    ASSERT_FALSE(ec) << ec.message();
    auto it = resp.headers.find("x-custom"); // case-insensitive
    if (it != resp.headers.end())
    {
      has_custom = true;
      custom_value = it->second;
    }
  });
  ctx.run();

  ASSERT_TRUE(has_custom);
  EXPECT_EQ(custom_value, "FooBar");
}

TEST(http_client_test, handles_connection_refused)
{
  // Use a port that no one is listening on (allocate one, then don't start anything).
  const auto port = next_port();

  asio::io_context ctx;
  bool threw = false;
  try
  {
    http::client cli(ctx, "127.0.0.1", port); // sync_connect throws
  } catch (const core::http_error &)
  {
    threw = true;
  }

  EXPECT_TRUE(threw);
}

TEST(http_client_test, times_out_when_server_silent)
{
  const auto port = next_port();
  raw_server_ignore srv(port);

  asio::io_context ctx;
  http::http_configuration cfg { .io_timeout = std::chrono::milliseconds(200) };
  http::client cli(ctx, "127.0.0.1", port, http::client::connect_type::sync, std::move(cfg));

  boost::system::error_code got;
  cli.get("/", {}, [&](boost::system::error_code ec, const auto &) { got = ec; });
  ctx.run();

  EXPECT_EQ(got, asio::error::timed_out);
}
