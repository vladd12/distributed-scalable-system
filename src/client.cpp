#include <http/client.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
  boost::asio::io_context ctx;
  http::http_client client(ctx, "localhost", 8080);

  client.get("/health", {}, //
      [](boost::system::error_code ec, const http::response &resp) {
        if (ec)
          std::cout << "error code: " << ec.what() << '\n';
        std::cout << "code: " << resp.status_code << ", body: " << resp.body << '\n';
      });

  ctx.run();
  return 0;
}
