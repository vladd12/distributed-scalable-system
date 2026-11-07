#pragma once

#include <boost/asio.hpp>
#include <http/server.hpp>
//
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace http_test
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/// @brief Monotonic port allocator so parallel test bodies never collide.
inline std::uint16_t next_port()
{
  static std::atomic<std::uint16_t> counter { 19100 };
  return counter.fetch_add(1);
}

/// @brief Reads from `sock` until EOF or error. Blocks.
inline std::string read_until_eof(tcp::socket &sock)
{
  boost::system::error_code ec;
  std::string result;
  char buf[4096];
  for (;;)
  {
    std::size_t n = sock.read_some(asio::buffer(buf), ec);
    if (ec)
      break;
    result.append(buf, n);
  }
  return result;
}

/// @brief Sends raw `request` bytes and returns whatever the server sends
///        until it closes. Assumes the request has `Connection: close`.
inline std::string raw_http_request(std::uint16_t port, std::string_view request)
{
  asio::io_context ctx;
  tcp::socket sock(ctx);
  sock.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
  asio::write(sock, asio::buffer(request.data(), request.size()));
  return read_until_eof(sock);
}

/// @brief Raw TCP server that answers every connection with a fixed payload.
class raw_server
{
protected:
  asio::io_context m_ctx;
  tcp::acceptor m_acceptor;
  std::string m_response;
  std::thread m_thread;
  std::atomic<bool> m_stopped { false };

  void do_accept()
  {
    m_acceptor.async_accept([this](boost::system::error_code ec, tcp::socket sock) {
      if (ec)
        return;
      auto shared = std::make_shared<tcp::socket>(std::move(sock));
      auto buf = std::make_shared<asio::streambuf>();
      // Read request first; otherwise shutdown/close with unread data on Windows
      // produces RST instead of FIN, and the client sees ECONNRESET.
      asio::async_read_until(*shared, *buf, "\r\n\r\n", [this, shared, buf](boost::system::error_code ec, std::size_t) {
        if (ec)
          return;
        write(shared);
      });
      if (!m_stopped)
        do_accept();
    });
  }

  virtual void write(std::shared_ptr<tcp::socket> shared)
  {
    asio::async_write(*shared, asio::buffer(m_response), [shared](boost::system::error_code, std::size_t) {
      boost::system::error_code ignored;
      shared->shutdown(tcp::socket::shutdown_send, ignored);
      shared->close(ignored);
    });
  }

public:
  raw_server(std::uint16_t port, std::string response)
      : m_acceptor(m_ctx, tcp::endpoint(tcp::v4(), port)), m_response(std::move(response))
  {
    do_accept();
    m_thread = std::thread([this] {
      try
      {
        m_ctx.run();
      } catch (...)
      {
      }
    });
  }

  virtual ~raw_server()
  {
    m_stopped = true;
    m_ctx.stop();
    if (m_thread.joinable())
      m_thread.join();
  }
};

class raw_server_ignore final : public raw_server
{
private:
  std::vector<std::shared_ptr<tcp::socket>> m_hung; ///< Sockets held open to force client timeout.
  std::mutex m_hung_mutex;                          ///< Guards `m_hung` (dtor may race with io thread).

  void write(std::shared_ptr<tcp::socket> shared) override
  {
    // Keep the socket alive forever (until server is destroyed) so the
    // client observes a timeout instead of EOF.
    std::lock_guard<std::mutex> lock(m_hung_mutex);
    m_hung.push_back(std::move(shared));
  }

public:
  raw_server_ignore(std::uint16_t port) : raw_server(port, "")
  {
  }
};

/// @brief Wraps http::server in a background thread.
class server_fixture
{
  asio::io_context m_ctx;
  std::unique_ptr<::http::server> m_server;
  std::thread m_thread;
  std::uint16_t m_port;

public:
  explicit server_fixture(std::uint16_t port) : m_port(port)
  {
    m_server = std::make_unique<::http::server>(m_ctx, port);
  }

  void start()
  {
    m_thread = std::thread([this] {
      try
      {
        m_ctx.run();
      } catch (...)
      {
      }
    });
  }

  void stop()
  {
    m_ctx.stop();
    if (m_thread.joinable())
      m_thread.join();
  }

  inline ::http::server &get() noexcept
  {
    return *m_server;
  }

  inline std::uint16_t port() const noexcept
  {
    return m_port;
  }

  inline ~server_fixture()
  {
    stop();
  }
};

} // namespace http_test
