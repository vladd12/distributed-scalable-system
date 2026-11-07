#include "http/client.hpp"

#include <http/session.hpp>
#include <utils/strings.hpp>

namespace http
{

// ---------------------------------------------------------------------------
// http::client::client_session
// ---------------------------------------------------------------------------

/// @brief   One-shot HTTP client session: connects, sends one request, receives
///          one response, then optionally keeps the socket open for another.
/// @details All HTTP state-machine logic lives in http::session. This class
///          only wires up: resolver, connect strategy (sync/async), the
///          request callback, and the three hooks.
class client::client_session final : public session<client_session, response, request>
{
private:
  tcp::resolver m_resolver;               ///< DNS resolver used for sync/async connect
  response_callback m_callback;           ///< Callback of the in-flight request (one at a time)
  on_connect_callback m_on_conn_callback; ///< Callback awaited by on_connected()
  std::string m_host;                     ///< Remote host (kept for Host header and reconnect)
  std::uint16_t m_port;                   ///< Remote port (kept for Host header)
  connect_type m_conn_type;               ///< Whether connect() is done sync or async

  /// @brief   Private ctor of the client session object.
  /// @details Use create() for client session object creation.
  client_session(asio::io_context &ctx, const std::string &host, const std::uint16_t port, const connect_type conn_type,
      http_configuration cfg)
      : session(tcp::socket(ctx), std::move(cfg)), m_resolver(ctx), m_host(host), m_port(port), m_conn_type(conn_type)
  {
  }

public:
  /// @brief Factory: creates a session bound to the given endpoint.
  /// @param ctx       io_context that will drive async operations.
  /// @param host      Remote host.
  /// @param port      Remote port.
  /// @param conn_type sync or async connect strategy.
  /// @param cfg       Session configuration.
  [[nodiscard]] static pointer create(asio::io_context &ctx, const std::string &host, const std::uint16_t port,
      const connect_type conn_type, http_configuration cfg)
  {
    return std::shared_ptr<client_session>(new client_session(ctx, host, port, conn_type, std::move(cfg)));
  }

  /// @brief Returns the connect strategy chosen at object construction.
  [[nodiscard]] inline connect_type get_connect_type() const noexcept
  {
    return m_conn_type;
  }

  /// @brief  Synchronously resolves and connects. Blocks the calling thread.
  /// @throws core::http_error on resolve or connect failure.
  void sync_connect()
  {
    boost::system::error_code ec;
    auto results = m_resolver.resolve(m_host, std::to_string(m_port), ec);
    if (ec)
    {
      shutdown();
      throw core::http_error(ec.message());
    }
    asio::connect(m_socket, results, ec);
    if (ec)
    {
      shutdown();
      throw core::http_error(ec.message());
    }
  }

  /// @brief   Asynchronously resolves and connects.
  /// @param   handle Invoked once with the final status (resolve or connect error).
  /// @details Caller must pump io_context for the operation to make progress.
  void async_connect(on_connect_callback handle)
  {
    m_on_conn_callback = std::move(handle);
    m_resolver.async_resolve(m_host, std::to_string(m_port), //
        [self = shared_from_this()](boost::system::error_code ec, tcp::resolver::results_type results) {
          // on resolving
          if (ec)
          {
            if (self->m_on_conn_callback)
              self->m_on_conn_callback(ec);
          }
          else
          {
            asio::async_connect(self->m_socket, results, //
                [self = self](boost::system::error_code ec, tcp::endpoint) {
                  // on connection
                  if (self->m_on_conn_callback)
                    self->m_on_conn_callback(ec);
                });
          }
        });
  }

  /// @brief   Sends one request; the response is delivered via cb.
  /// @param   req Request to send (moved).
  /// @param   cb  Response callback (may be empty; then the response is dropped).
  /// @details Fills in default Host and Connection headers if absent, then
  ///          hands the write off to the base state machine. Only one request
  ///          can be in flight at a time; the previous callback is overwritten.
  void async_request(write_t req, response_callback cb)
  {
    if (!is_open())
    {
      if (cb)
        cb(asio::error::not_connected, {});
      return;
    }

    if (!req.headers.contains("Host"))
      req.headers["Host"] = m_host + ":" + std::to_string(m_port);
    if (!req.headers.contains("Connection"))
      req.headers["Connection"] = "Keep-Alive";

    m_callback = std::move(cb);
    do_write(req);
  }

  // --- hooks from http::session ---

  /// @brief   Hook: a full response has been parsed. Invokes the pending callback.
  /// @details Inspects the Connection header to decide keep-alive for the next
  ///          request. Callback is cleared before invocation, so a new request
  ///          may be started from within it (chained requests).
  void on_message_parsed(read_t resp)
  {
    m_keep_alive = false;
    auto it = resp.headers.find("Connection");
    if (it != resp.headers.end())
      m_keep_alive = utils::to_lower(it->second) != "close";

    auto cb = std::move(m_callback);
    m_callback = nullptr;
    if (cb)
      cb({}, std::move(resp));

    if (!m_keep_alive)
      shutdown();
  }

  /// @brief Hook: request was fully written. Switch to reading the response.
  void on_write_complete()
  {
    do_read();
  }

  /// @brief Hook: any failure. Shuts the socket down and delivers the error.
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

client::client(asio::io_context &ctx, const std::string &host, const std::uint16_t port, const connect_type conn_type,
    http_configuration cfg)
    : m_host(host), m_session(client_session::create(ctx, host, port, conn_type, std::move(cfg)))
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
  request req { { method::GET, path, "HTTP/1.1" }, headers, {} };
  async_request(std::move(req), std::move(handler));
}

void client::post(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler)
{
  request req { { method::POST, path, "HTTP/1.1" }, headers, body };
  async_request(std::move(req), std::move(handler));
}

void client::put(const std::string &path, const headers_t &headers, const std::string &body, response_callback handler)
{
  request req { { method::PUT, path, "HTTP/1.1" }, headers, body };
  async_request(std::move(req), std::move(handler));
}

void client::delete_(const std::string &path, const headers_t &headers, response_callback handler)
{
  request req { { method::DELETE_, path, "HTTP/1.1" }, headers, {} };
  async_request(std::move(req), std::move(handler));
}

void client::async_request(request req, response_callback handler)
{
  m_session->async_request(std::move(req), std::move(handler));
}

} // namespace http
