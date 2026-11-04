#include "http/server.hpp"

#include <core/errors.hpp>
#include <http/session.hpp>

namespace http
{

// ---------------------------------------------------------------------------
// http::server_session
// ---------------------------------------------------------------------------

/// @brief   One-shot HTTP server session: accepts a socket, reads a request,
///          dispatches it via the route handler, writes the response, then either
///          loops (keep-alive) or shuts down (Connection: close).
/// @details All HTTP state-machine logic lives in http::session. This class
///          only wires up: the request handler and the three hooks.
class server_session final : public session<server_session, request, response>
{
private:
  request_handler m_handler; ///< User-provided request dispatcher

  /// @brief   Private ctor of the server session object.
  /// @details Use create() for server session object creation.
  server_session(tcp::socket socket, request_handler handler, http_configuration cfg)
      : session(std::move(socket), std::move(cfg)), m_handler(std::move(handler))
  {
  }

public:
  /// @brief Factory: wraps an accepted socket in a server_session.
  /// @param socket  Socket returned by acceptor (moved in).
  /// @param handler Request handler to dispatch to.
  /// @param cfg     Session configuration.
  [[nodiscard]] static pointer create(tcp::socket socket, request_handler handler, http_configuration cfg = {})
  {
    return std::shared_ptr<server_session>(new server_session(std::move(socket), std::move(handler), std::move(cfg)));
  }

  /// @brief Entry point: starts the first read cycle.
  void start()
  {
    do_read();
  }

  // --- hooks from http::session ---

  /// @brief   Hook: a full request has been parsed. Dispatches it and writes the response.
  /// @details Sets keep-alive based on the incoming Connection header, catches
  ///          exceptions from the handler and turns them into 500 responses
  ///          (which force close), and echoes the effective Connection
  ///          decision in the response.
  void on_message_parsed(read_t req)
  {
    auto it = req.headers.find("connection");
    m_keep_alive = !(it != req.headers.end() && it->second == "close");

    write_t resp;
    try
    {
      resp = m_handler(req);
    } catch (const std::exception &ex)
    {
      resp = response::text(status_code::INTERNAL_SERVER_ERROR, ex.what());
      m_keep_alive = false;
    }

    resp.headers["Connection"] = m_keep_alive ? "keep-alive" : "close";
    do_write(resp);
  }

  /// @brief Hook: response was fully written. Either loop back to read or close.
  void on_write_complete()
  {
    if (m_keep_alive)
      do_read();
    else
      shutdown();
  }

  /// @brief Hook: any failure. Shuts the socket down (server side has nothing to report).
  void on_error(boost::system::error_code)
  {
    shutdown();
  }
};

// ---------------------------------------------------------------------------
// http::server
// ---------------------------------------------------------------------------

server::server(asio::io_context &ctx, const std::uint16_t port)
    : m_acceptor(ctx, tcp::endpoint(asio::ip::make_address("0.0.0.0"), port))
{
  m_default_handler = [](const request &) { return response::text(status_code::NOT_FOUND, "Not Found"); };
  do_accept();
}

void server::route(method m, const std::string &path, request_handler handler)
{
  m_routes[route_key { m, path }] = std::move(handler);
}

void server::set_default_handler(request_handler handler)
{
  m_default_handler = std::move(handler);
}

void server::do_accept()
{
  m_acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
    if (ec == asio::error::operation_aborted)
      return;
    if (!ec)
    {
      auto handler = [this](const request &req) { return dispatch(req); };
      server_session::create(std::move(socket), handler)->start();
    }
    do_accept();
  });
}

response server::dispatch(const request &req)
{
  auto it = m_routes.find(route_key { req.line.method_type, req.line.path });
  if (it != m_routes.end())
    return it->second(req);
  return m_default_handler(req);
}

std::size_t server::route_key_hash::operator()(const route_key &key) const noexcept
{
  static constexpr std::size_t hash_constant = 0x9e3779b9;
  std::size_t h1 = std::hash<int> {}(static_cast<int>(key.method_type));
  std::size_t h2 = std::hash<std::string> {}(key.path);
  return h1 ^ (h2 + hash_constant + (h1 << 6) + (h1 >> 2)); // better hashing
}

} // namespace http
