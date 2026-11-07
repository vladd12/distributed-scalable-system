#include "http_test_helpers.hpp"

#include <gtest/gtest.h>
#include <http/client.hpp>
#include <http/server.hpp>

using namespace http_test;

TEST(http_integration_test, get_roundtrip)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/status", [](const auto &) { return http::response::text(http::status_code::OK, "ok"); });
  srv.start();

  asio::io_context cli_ctx;
  http::client cli(cli_ctx, "127.0.0.1", srv.port());

  bool done = false;
  std::string body;

  cli.get("/status", {}, [&](boost::system::error_code ec, const http::response &resp) {
    ASSERT_FALSE(ec) << ec.message();
    body = resp.body;
    done = true;
  });
  cli_ctx.run();

  ASSERT_TRUE(done);
  EXPECT_EQ(body, "ok");
}

TEST(http_integration_test, post_body_roundtrip)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::POST, "/echo",
      [](const http::request &req) { return http::response::text(http::status_code::OK, req.body); });
  srv.start();

  asio::io_context cli_ctx;
  http::client cli(cli_ctx, "127.0.0.1", srv.port());

  std::string got;
  cli.post("/echo", {}, "hello world", [&](boost::system::error_code ec, const http::response &resp) {
    ASSERT_FALSE(ec) << ec.message();
    got = resp.body;
  });
  cli_ctx.run();

  EXPECT_EQ(got, "hello world");
}

// THE critical test: two sequential requests on the same session.
// Fails if keep-alive is broken (e.g. the cyrillic `Сonnection` bug).
TEST(http_integration_test, chained_requests_keep_alive)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/a", [](const auto &) { return http::response::text(http::status_code::OK, "a"); });
  srv.get().route(
      http::method::GET, "/b", [](const auto &) { return http::response::text(http::status_code::OK, "b"); });
  srv.start();

  asio::io_context cli_ctx;
  http::client cli(cli_ctx, "127.0.0.1", srv.port());

  int completions = 0;
  std::string second_body;

  cli.get("/a", {}, [&](boost::system::error_code ec, const http::response &) {
    ASSERT_FALSE(ec) << "first request failed: " << ec.message();
    ++completions;

    cli.get("/b", {}, [&](boost::system::error_code ec2, const http::response &r2) {
      ASSERT_FALSE(ec2) << "second request failed: " << ec2.message();
      second_body = r2.body;
      ++completions;
    });
  });
  cli_ctx.run();

  EXPECT_EQ(completions, 2);
  EXPECT_EQ(second_body, "b");
}

TEST(http_integration_test, multiple_requests_sequential)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/ping", [](const auto &) { return http::response::text(http::status_code::OK, "pong"); });
  srv.start();

  asio::io_context cli_ctx;
  http::client cli(cli_ctx, "127.0.0.1", srv.port());

  int count = 0;
  std::function<void()> do_one;
  do_one = [&] {
    if (count >= 5)
      return;
    cli.get("/ping", {}, [&](boost::system::error_code ec, const http::response &r) {
      ASSERT_FALSE(ec) << "request #" << count << ": " << ec.message();
      ASSERT_EQ(r.body, "pong");
      ++count;
      do_one();
    });
  };
  do_one();
  cli_ctx.run();

  EXPECT_EQ(count, 5);
}

TEST(http_integration_test, large_body_within_limit)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::POST, "/echo",
      [](const http::request &req) { return http::response::text(http::status_code::OK, req.body); });
  srv.start();

  // 512 KiB body, under default max_body_size = 1 MiB
  std::string payload(512 * 1024, 'x');

  asio::io_context cli_ctx;
  http::client cli(cli_ctx, "127.0.0.1", srv.port());

  std::size_t got = 0;
  cli.post("/echo", {}, payload, [&](boost::system::error_code ec, const http::response &r) {
    ASSERT_FALSE(ec) << ec.message();
    got = r.body.size();
  });
  cli_ctx.run();

  EXPECT_EQ(got, payload.size());
}
