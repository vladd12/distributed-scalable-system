#include <gtest/gtest.h>
#include <http/client.hpp>
#include <http/server.hpp>

TEST(http_client_test, chained_requests_keep_socket)
{
  boost::asio::io_context ctx;
  http::server srv(ctx, 18080);
  srv.route(http::method::GET, "/a", [](const auto &) { return http::response::text(http::status_code::OK, "a"); });
  srv.route(http::method::GET, "/b", [](const auto &) { return http::response::text(http::status_code::OK, "b"); });

  std::thread server_thread([&] { ctx.run(); });

  boost::asio::io_context cli_ctx;
  http::client cli(cli_ctx, "localhost", 18080);
  int completed = 0;
  std::string second_body;

  cli.get("/a", {}, [&](boost::system::error_code ec, const http::response &resp) {
    ASSERT_FALSE(ec) << ec.message();
    ++completed;
    cli.get("/b", {}, [&](boost::system::error_code ec2, const http::response &resp2) {
      ASSERT_FALSE(ec2) << ec2.message();
      second_body = resp2.body;
      ++completed;
    });
  });
  cli_ctx.run();

  ctx.stop();
  server_thread.join();

  ASSERT_EQ(completed, 2);
  ASSERT_EQ(second_body, "b");
}
