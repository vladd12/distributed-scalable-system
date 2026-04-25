#include <http/server.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
  boost::asio::io_context ctx;
  http::http_server server(ctx, 8080);

  server.route(http::method::GET, "/health", //
      [](const http::request &) {            //
        std::cout << "incoming request\n";
        return http::response::json(http::status_code::OK, R"({"status":"ok"})");
      });
  server.route(http::method::POST, "/data",            //
      [](const http::request &req) -> http::response { //
        return http::response::text(http::status_code::OK, req.body);
      });

  ctx.run();
  return 0;
}
