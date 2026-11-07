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

TEST(http_integration_test, put_roundtrip)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::PUT, "/kv/k",
      [](const http::request &req) { return http::response::text(http::status_code::OK, req.body); });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());
  std::string got;
  cli.put("/kv/k", {}, "value", [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    got = r.body;
  });
  ctx.run();
  EXPECT_EQ(got, "value");
}

TEST(http_integration_test, delete_roundtrip)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::DELETE_, "/kv/k",
      [](const auto &) { return http::response::simple(http::status_code::NO_CONTENT); });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());
  int status = 0;
  bool done = false;
  cli.delete_("/kv/k", {}, [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    status = static_cast<int>(r.line.status_code);
    done = true;
  });
  ctx.run();
  EXPECT_TRUE(done);
  EXPECT_EQ(status, 204);
}

TEST(http_integration_test, multiple_clients_concurrently)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/ping", [](const auto &) { return http::response::text(http::status_code::OK, "pong"); });
  srv.start();

  constexpr int N = 5;
  std::atomic<int> completed { 0 };
  std::vector<std::thread> threads;

  for (int i = 0; i < N; ++i)
  {
    threads.emplace_back([&, i] {
      asio::io_context ctx;
      http::client cli(ctx, "127.0.0.1", srv.port());
      cli.get("/ping", {}, [&](auto ec, const auto &r) {
        ASSERT_FALSE(ec) << "client " << i << ": " << ec.message();
        ASSERT_EQ(r.body, "pong");
        ++completed;
      });
      ctx.run();
    });
  }
  for (auto &t : threads)
    t.join();

  EXPECT_EQ(completed.load(), N);
}

TEST(http_integration_test, large_response_within_limit)
{
  server_fixture srv(next_port());
  const std::string payload(512 * 1024, 'y');
  srv.get().route(http::method::GET, "/blob",
      [payload](const auto &) { return http::response::text(http::status_code::OK, payload); });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());
  std::size_t got = 0;
  cli.get("/blob", {}, [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    got = r.body.size();
  });
  ctx.run();
  EXPECT_EQ(got, payload.size());
}

TEST(http_integration_test, custom_headers_both_directions)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::POST, "/upload", [](const http::request &req) {
    // Server sees X-Block-Id from client
    auto it = req.headers.find("x-block-id");
    std::string block_id = (it != req.headers.end()) ? it->second : "<missing>";
    auto resp = http::response::text(http::status_code::OK, "stored:" + block_id);
    resp.headers["X-Checksum"] = "deadbeef";
    return resp;
  });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());

  std::string body;
  std::string checksum;
  cli.post("/upload", { { "X-Block-Id", "block-42" } }, "data", [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    body = r.body;
    auto it = r.headers.find("x-checksum");
    if (it != r.headers.end())
      checksum = it->second;
  });
  ctx.run();
  EXPECT_EQ(body, "stored:block-42");
  EXPECT_EQ(checksum, "deadbeef");
}

TEST(http_integration_test, utf8_in_body_and_headers)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::POST, "/echo", [](const http::request &req) {
    auto it = req.headers.find("x-filename");
    std::string name = (it != req.headers.end()) ? it->second : "";
    return http::response::text(http::status_code::OK, name + ":" + req.body);
  });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());
  const std::string payload = "Привет, мир!";
  std::string got;
  cli.post("/echo", { { "X-Filename", "файл.bin" } }, payload, [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    got = r.body;
  });
  ctx.run();
  EXPECT_EQ(got, "файл.bin:Привет, мир!");
}

TEST(http_integration_test, empty_post_body)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::POST, "/touch", [](const http::request &req) {
    return http::response::json(http::status_code::OK, "{\"body_size\":" + std::to_string(req.body.size()) + "}");
  });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());
  std::string got;
  cli.post("/touch", {}, "", [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    got = r.body;
  });
  ctx.run();
  EXPECT_EQ(got, "{\"body_size\":0}");
}

TEST(http_server_test, method_mismatch_returns_404)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/foo", [](const auto &) { return http::response::text(http::status_code::OK, "foo"); });
  srv.start();

  auto resp = raw_http_request(
      srv.port(), "POST /foo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
  EXPECT_NE(resp.find("404 Not Found"), std::string::npos) << resp;
}

TEST(http_integration_test, client_does_not_override_custom_host_header)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::GET, "/whoami", [](const http::request &req) {
    auto it = req.headers.find("host");
    std::string host = (it != req.headers.end()) ? it->second : "<missing>";
    return http::response::text(http::status_code::OK, host);
  });
  srv.start();

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());
  std::string got;
  cli.get("/whoami", { { "Host", "custom.example.com" } }, [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    got = r.body;
  });
  ctx.run();
  EXPECT_EQ(got, "custom.example.com"); // NOT 127.0.0.1:PORT
}

TEST(http_integration_test, binary_body_with_nulls)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::POST, "/echo",
      [](const http::request &req) { return http::response::text(http::status_code::OK, req.body); });
  srv.start();

  std::string payload;
  for (int i = 0; i < 1000; ++i)
    payload.push_back(static_cast<char>(i % 256)); // includes 0x00

  asio::io_context ctx;
  http::client cli(ctx, "127.0.0.1", srv.port());
  std::string got;
  cli.post("/echo", {}, payload, [&](auto ec, const auto &r) {
    ASSERT_FALSE(ec) << ec.message();
    got = r.body;
  });
  ctx.run();
  EXPECT_EQ(got.size(), payload.size());
  EXPECT_TRUE(std::equal(got.begin(), got.end(), payload.begin()));
}
