#include "http/client.hpp"

#include <http/session.hpp>

constexpr inline bool use_async = false;

namespace http
{

// ---------------------------------------------------------------------------
// http::client::client_session
// ---------------------------------------------------------------------------

class client::client_session final : public session<client_session, response, request>
{
private:
  tcp::resolver m_resolver;
  response_callback m_callback;
  on_connect_callback m_on_conn_callback;
  std::string m_host;
  std::uint16_t m_port;
  connect_type m_conn_type;

  /// @brief Ctor of the client session object.
  client_session(asio::io_context &ctx, const std::string &host, const std::uint16_t port, const connect_type conn_type,
      http_configuration cfg)
      : session(tcp::socket(ctx), std::move(cfg)), m_resolver(ctx), m_host(host), m_port(port), m_conn_type(conn_type)
  {
  }

public:
  using read_t = response;
  using write_t = request;
  using base_t = session<client_session, read_t, write_t>;
  using pointer = std::shared_ptr<client_session>;

  [[nodiscard]] static pointer create(asio::io_context &ctx, const std::string &host, const std::uint16_t port,
      const connect_type conn_type, http_configuration cfg = {})
  {
    return std::shared_ptr<client_session>(new client_session(ctx, host, port, conn_type, std::move(cfg)));
  }

  [[nodiscard]] inline connect_type get_connect_type() const noexcept
  {
    return m_conn_type;
  }

  /// @brief Blocks the caller thread; throws core::http_error on failure.
  void sync_connect()
  {
    boost::system::error_code ec;
    auto results = m_resolver.resolve(m_host, std::to_string(m_port), ec);
    if (ec)
    {
      base_t::shutdown();
      throw core::http_error(ec.message());
    }
    asio::connect(m_socket, results, ec);
    if (ec)
    {
      shutdown();
      throw core::http_error(ec.message());
    }
  }

  void async_connect(on_connect_callback handle)
  {
    m_on_conn_callback = std::move(handle);
    m_resolver.async_resolve(m_host, std::to_string(m_port), //
        [self = shared_from_this()](boost::system::error_code ec, tcp::resolver::results_type results) {
          // on resolving
          if (ec)
          {
            self->m_on_conn_callback(std::move(ec));
            return;
          }
          asio::async_connect(self->m_socket, results, //
              [self = self](boost::system::error_code ec, tcp::endpoint) {
                // on connection
                self->m_on_conn_callback(std::move(ec));
              });
        });
  }

  void async_request(write_t req, response_callback cb)
  {
    if (!is_open())
    {
      if (cb)
        cb(asio::error::not_connected, {});
      return;
    }

    if (!req.headers.contains("host") && !req.headers.contains("Host"))
      req.headers["Host"] = m_host + ":" + std::to_string(m_port);
    if (!req.headers.contains("connection") && !req.headers.contains("Connection"))
      req.headers["Connection"] = "keep-alive";

    m_callback = std::move(cb);
    do_write(req);
  }

  // --- hooks from http::session ---

  void on_message_parsed(read_t resp)
  {
    auto it = resp.headers.find("connection");
    m_keep_alive = !(it != resp.headers.end() && it->second == "close");

    auto cb = std::move(m_callback);
    m_callback = nullptr;
    if (cb)
      cb({}, std::move(resp));
  }

  void on_write_complete()
  {
    do_read();
  }

  void on_error(boost::system::error_code ec)
  {
    auto cb = std::move(m_callback);
    m_callback = nullptr;
    shutdown();
    if (cb)
      cb(ec, {});
  }
};

// ---------------------------------------------------------------------------
// http::client
// ---------------------------------------------------------------------------

client::client(asio::io_context &ctx, const std::string &host, const std::uint16_t port, const connect_type conn_type)
    : m_host(host), m_session(client_session::create(ctx, host, port, conn_type))
{
  if (m_session->get_connect_type() == connect_type::sync)
    m_session->sync_connect();
}

void client::on_connected(on_connect_callback handler)
{
  if (m_session->get_connect_type() == connect_type::async)
    m_session->async_connect(std::move(handler));
}

void client::get(const std::string &path, const headers_t &headers, response_callback handler)
{
  request req { method::GET, path, "HTTP/1.1", headers };
  async_request(std::move(req), std::move(handler));
}

void client::post(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler)
{
  request req { method::POST, path, "HTTP/1.1", headers, body };
  async_request(std::move(req), std::move(handler));
}

void client::put(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler)
{
  request req { method::PUT, path, "HTTP/1.1", headers, body };
  async_request(std::move(req), std::move(handler));
}

void client::delete_(const std::string &path, const headers_t &headers, response_callback handler)
{
  request req { method::DELETE_, path, "HTTP/1.1", headers };
  async_request(std::move(req), std::move(handler));
}

void client::async_request(request req, response_callback handler)
{
  m_session->async_request(std::move(req), std::move(handler));
}

} // namespace http
