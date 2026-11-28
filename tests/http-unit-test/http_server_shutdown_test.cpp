#include "http_test_helpers.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <http/client.hpp>
#include <http/server.hpp>
#include <thread>

using namespace http_test;

TEST(http_server_shutdown_test, stops_accepting_new_connections)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/ping", [](const auto &) { return http::response::text(http::status_code::OK, "pong"); });
  srv.start();

  // First request: works.
  {
    asio::io_context ctx;
    http::client cli(ctx, "127.0.0.1", srv.port());
    std::string body;
    cli.get("/ping", {}, [&](auto ec, const auto &r) {
      ASSERT_FALSE(ec) << ec.message();
      body = r.body;
    });
    ctx.run();
    ASSERT_EQ(body, "pong");
  }

  // Stop the server.
  srv.get().stop();
  srv.stop(); // ждём, пока io_context домелет pending ops

  // New connection must fail.
  asio::io_context ctx;
  bool connected = true;
  try
  {
    http::client cli(ctx, "127.0.0.1", srv.port());
  } catch (const core::http_error &)
  {
    connected = false;
  }
  EXPECT_FALSE(connected);
}

TEST(http_server_shutdown_test, active_sessions_drops_to_zero)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/ping", [](const auto &) { return http::response::text(http::status_code::OK, "pong"); });
  srv.start();

  // Keep-alive client: session stays open between requests.
  asio::io_context cli_ctx;
  http::client cli(cli_ctx, "127.0.0.1", srv.port());

  std::atomic<int> completed { 0 };
  cli.get("/ping", {}, [&](auto ec, const auto &) {
    ASSERT_FALSE(ec) << ec.message();
    ++completed;
  });
  cli_ctx.run();
  ASSERT_EQ(completed.load(), 1);

  // Give server io_context a moment to register the session.
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_GE(srv.get().active_sessions(), 1u);

  srv.get().stop();

  // Close client side so the server session can be cleaned up.
  cli.close();
  cli_ctx.run();
  srv.stop();

  EXPECT_EQ(srv.get().active_sessions(), 0u);
}

TEST(http_server_shutdown_test, stop_is_idempotent)
{
  server_fixture srv(next_port());
  srv.start();
  srv.get().stop();
  EXPECT_NO_THROW(srv.get().stop());
  EXPECT_NO_THROW(srv.get().stop());
  srv.stop();
}

TEST(http_server_shutdown_test, is_stopped_reflects_state)
{
  server_fixture srv(next_port());
  srv.start();
  EXPECT_FALSE(srv.get().is_stopped());
  srv.get().stop();
  EXPECT_TRUE(srv.get().is_stopped());
  srv.stop();
}
