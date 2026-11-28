#include "http_test_helpers.hpp"

#include <gtest/gtest.h>
#include <http/client.hpp>

using namespace http_test;

TEST(http_client_shutdown_test, close_without_pending_does_not_call_callback)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/ping", [](const auto &) { return http::response::text(http::status_code::OK, "pong"); });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());

  bool any_callback = false;
  cli.get("/ping", {}, [&](auto ec, const auto &) {
    ASSERT_FALSE(ec);
    any_callback = true;
  });
  ctx.run();
  ASSERT_TRUE(any_callback);

  // No pending request — close should not fire any callback.
  EXPECT_NO_THROW(cli.close());
  EXPECT_TRUE(cli.is_closed());
}

TEST(http_client_shutdown_test, close_cancels_pending_request)
{
  const auto port = next_port();
  raw_server_ignore srv(port); // accepts but never responds

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", port);

  boost::system::error_code got;
  cli.get("/", {}, [&](boost::system::error_code ec, const auto &) { got = ec; });

  // close() before ctx.run() — callback fires with operation_aborted.
  cli.close();

  ctx.run();
  EXPECT_EQ(got, asio::error::operation_aborted);
  EXPECT_TRUE(cli.is_closed());
}

TEST(http_client_shutdown_test, close_is_idempotent)
{
  const auto port = next_port();
  raw_server_ignore srv(port);

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", port);

  EXPECT_NO_THROW(cli.close());
  EXPECT_NO_THROW(cli.close());
  EXPECT_TRUE(cli.is_closed());
}

TEST(http_client_shutdown_test, request_after_close_returns_not_connected)
{
  const auto port = next_port();
  raw_server_ignore srv(port);

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", port);
  cli.close();

  boost::system::error_code got;
  cli.get("/", {}, [&](boost::system::error_code ec, const auto &) { got = ec; });

  ctx.run();
  EXPECT_EQ(got, asio::error::not_connected);
}
