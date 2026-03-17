#include <http/server.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
  boost::asio::io_context ctx;
  http::http_server server(ctx, 8080);

  server.route(http::method::GET, "/health", //
      [](const http::request &) {            //
        std::cout << "incoming request\n";
        return http::response::json(200, R"({"status":"ok"})");
      });

  ctx.run();
  return 0;
}
