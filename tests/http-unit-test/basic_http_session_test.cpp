#include <gtest/gtest.h>
#include <http/basic_http_session.hpp>
#include <http/request.hpp>
#include <http/response.hpp>

TEST(basic_http_session_test, compile_client_test)
{
  struct client_role
  {
    typedef http::response read_t;
    typedef http::request write_t;
  };

  boost::asio::io_context ctx;
  auto session = http::basic_http_session<client_role>::create(ctx);
  ASSERT_TRUE(session);
  session->async_read();
}

TEST(basic_http_session_test, compile_server_test)
{
  struct server_role;
  using http_server_session = http::basic_http_session<server_role>;

  struct server_role
  {
    typedef http::request read_t;
    typedef http::response write_t;

    http_server_session::error_handler m_error_handler = //
        [](const boost::system::error_code &ec, http_server_session::pointer session) {
          if (ec == http::asio::error::operation_aborted)
            return;

          session->shutdown();
        };
  };

  boost::asio::io_context ctx;
  auto session = http::basic_http_session<server_role>::create(ctx);
  ASSERT_TRUE(session);
  session->async_read();
}
