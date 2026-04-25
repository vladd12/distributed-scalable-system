#include <core/conversion.hpp>
#include <http/client.hpp>
#include <iostream>
#include <random>

namespace
{

inline char get_character(const std::size_t index)
{
  constexpr std::string_view str_container = "ABCDEFGHIJKLMNOPQRSTUVWXYZizj0k1l2m3n4o5p6q7ar8bs9ct+du/evfw=gxhy";
  constexpr std::size_t length = str_container.length();
  if (index >= length)
    return '0';
  return str_container[index];
}

inline std::string get_random_string(const std::size_t length)
{
  std::string result(length, '\n');
  std::random_device rd {};
  std::mt19937 generator { rd() };
  std::normal_distribution dist { 32.0, 4.0 };
  for (std::size_t index = 0; index < length; ++index)
  {
    result[index] = get_character(static_cast<std::size_t>(std::lround(dist(generator))));
  }

  return result;
}

}

int main(int argc, char *argv[])
{
  boost::asio::io_context ctx;
  http::http_client client(ctx, "localhost", 8080);

  const std::string data = get_random_string(1024 * 4);

  client.post("/data", {}, data, //
      [&client](boost::system::error_code ec, const http::response &resp) {
        if (ec)
        {
          std::cout << "error code: " << ec.what() << '\n';
          return;
        }

        std::cout << "code: " << core::to_underlying(resp.line.status_code) << '\n';
        if (resp.body.empty())
          std::cout << "empty body received\n";
        else
          std::cout << "body: " << resp.body << '\n';

        // chained request
        client.get("/health", {}, //
            [](boost::system::error_code ec, const http::response &resp) {
              if (ec)
              {
                std::cout << "error code: " << ec.what() << '\n';
                return;
              };
            });
      });

  // client.get("/health", {}, //
  //     [](boost::system::error_code ec, const http::response &resp) {
  //       if (ec)
  //         std::cout << "error code: " << ec.what() << '\n';
  //       std::cout << "code: " << resp.line.status_code << ", body: " << resp.body << '\n';
  //     });

  ctx.run();
  return 0;
}
