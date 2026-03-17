#include <http/client.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
  boost::asio::io_context ctx;
  http::http_client client(ctx);

  client.async_get("localhost", "8080", "/health", {}, //
      [](boost::system::error_code, const http::response &resp) {
        std::cout << "code: " << resp.status_code << ", body: " << resp.body << '\n';
      });

  ctx.run();
  return 0;
}
