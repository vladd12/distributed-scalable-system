#include "http_test_helpers.hpp"

#include <gtest/gtest.h>
#include <http/client.hpp>
#include <http/server.hpp>

using namespace http_test;

TEST(http_server_test, responds_to_registered_route)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/hello", [](const auto &) { return http::response::text(http::status_code::OK, "world"); });
  srv.start();

  auto resp = raw_http_request(srv.port(),
      "GET /hello HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Connection: close\r\n"
      "\r\n");

  EXPECT_NE(resp.find("200 OK"), std::string::npos) << resp;
  EXPECT_NE(resp.find("world"), std::string::npos) << resp;
}

TEST(http_server_test, returns_404_for_unknown_route)
{
  server_fixture srv(next_port());
  srv.start();

  auto resp = raw_http_request(srv.port(),
      "GET /nope HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Connection: close\r\n"
      "\r\n");

  EXPECT_NE(resp.find("404 Not Found"), std::string::npos) << resp;
}

TEST(http_server_test, exact_match_routing)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/foo", [](const auto &) { return http::response::text(http::status_code::OK, "foo"); });
  srv.start();

  // /foo matches
  auto r1 = raw_http_request(srv.port(), "GET /foo HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
  EXPECT_NE(r1.find("200 OK"), std::string::npos);

  // /foo/bar does not match (exact routing)
  auto r2 = raw_http_request(srv.port(), "GET /foo/bar HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
  EXPECT_NE(r2.find("404 Not Found"), std::string::npos) << r2;

  // /foo?x=1 does not match (query is part of the key)
  auto r3 = raw_http_request(srv.port(), "GET /foo?x=1 HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
  EXPECT_NE(r3.find("404 Not Found"), std::string::npos) << r3;
}

TEST(http_server_test, handler_exception_becomes_500)
{
  server_fixture srv(next_port());
  srv.get().route(
      http::method::GET, "/boom", [](const auto &) -> http::response { throw std::runtime_error("kaboom"); });
  srv.start();

  auto resp = raw_http_request(srv.port(), "GET /boom HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");

  EXPECT_NE(resp.find("500 Internal Server Error"), std::string::npos) << resp;
  EXPECT_NE(resp.find("kaboom"), std::string::npos) << resp;
}

TEST(http_server_test, rejects_oversized_content_length)
{
  server_fixture srv(next_port());
  srv.get().route(http::method::POST, "/data",
      [](const auto &req) { return http::response::text(http::status_code::OK, req.body); });
  srv.start();

  auto resp = raw_http_request(srv.port(),
      "POST /data HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 2000000\r\n"
      "Connection: close\r\n"
      "\r\n");

  EXPECT_NE(resp.find("413 Payload Too Large"), std::string::npos) << resp;
  EXPECT_NE(resp.find("connection: close"), std::string::npos) << resp;
}

TEST(http_server_test, rejects_oversized_headers)
{
  server_fixture srv(next_port());
  srv.start();

  // Single header value pushes total header size past max_header_size (16 KiB default).
  std::string request
      = "GET / HTTP/1.1\r\nHost: localhost\r\nX-Fill: " + std::string(20000, 'a') + "\r\nConnection: close\r\n\r\n";

  auto resp = raw_http_request(srv.port(), request);
  EXPECT_NE(resp.find("431 Request Header Fields Too Large"), std::string::npos) << resp;
  EXPECT_NE(resp.find("connection: close"), std::string::npos) << resp;
}

TEST(http_server_test, oversized_headers_response_has_content_length)
{
  server_fixture srv(next_port());
  srv.start();

  std::string request
      = "GET / HTTP/1.1\r\nHost: localhost\r\nX-Fill: " + std::string(20000, 'a') + "\r\nConnection: close\r\n\r\n";

  auto resp = raw_http_request(srv.port(), request);
  // The 431 body is the diagnostic string; it must have Content-Length.
  EXPECT_NE(resp.find("content-length:"), std::string::npos) << resp;
}

// ============================================================================
// Malformed request -> 400 Bad Request
// ============================================================================

TEST(http_server_test, malformed_request_line_returns_400)
{
  server_fixture srv(next_port());
  srv.start();

  auto resp = raw_http_request(srv.port(), "GARBAGE\r\nHost: localhost\r\nConnection: close\r\n\r\n");
  EXPECT_NE(resp.find("400 Bad Request"), std::string::npos) << resp;
  EXPECT_NE(resp.find("connection: close"), std::string::npos) << resp;
}

TEST(http_server_test, header_without_colon_returns_400)
{
  server_fixture srv(next_port());
  srv.start();

  auto resp
      = raw_http_request(srv.port(), "GET / HTTP/1.1\r\nHost: localhost\r\nBrokenHeader\r\nConnection: close\r\n\r\n");
  EXPECT_NE(resp.find("400 Bad Request"), std::string::npos) << resp;
}

TEST(http_server_test, duplicate_content_length_returns_400)
{
  server_fixture srv(next_port());
  srv.start();

  auto resp = raw_http_request(srv.port(),
      "POST / HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 5\r\n"
      "Content-Length: 10\r\n"
      "Connection: close\r\n"
      "\r\n");
  EXPECT_NE(resp.find("400 Bad Request"), std::string::npos) << resp;
}

TEST(http_server_test, invalid_content_length_value_returns_400)
{
  server_fixture srv(next_port());
  srv.start();

  auto resp = raw_http_request(srv.port(),
      "POST / HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: abc\r\n"
      "Connection: close\r\n"
      "\r\n");
  EXPECT_NE(resp.find("400 Bad Request"), std::string::npos) << resp;
}

TEST(http_server_test, invalid_content_length_trailing_garbage_returns_400)
{
  server_fixture srv(next_port());
  srv.start();

  auto resp = raw_http_request(srv.port(),
      "POST / HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 5x\r\n"
      "Connection: close\r\n"
      "\r\n");
  EXPECT_NE(resp.find("400 Bad Request"), std::string::npos) << resp;
}

// ============================================================================
// 400 body must NOT leak parser internals
// ============================================================================

TEST(http_server_test, bad_request_body_is_generic)
{
  server_fixture srv(next_port());
  srv.start();

  auto resp = raw_http_request(srv.port(), "GARBAGE\r\nHost: localhost\r\nConnection: close\r\n\r\n");

  // Parser message like "request line parsing error: ..." must NOT appear in body.
  EXPECT_EQ(resp.find("parsing error"), std::string::npos) << resp;
  EXPECT_EQ(resp.find("invalid 'Content-Length'"), std::string::npos) << resp;
}
