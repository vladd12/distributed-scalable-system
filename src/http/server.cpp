#include "http/server.hpp"

#include <core/errors.hpp>
#include <http/session.hpp>
#include <utils/strings.hpp>

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
  [[nodiscard]] static pointer create(tcp::socket socket, request_handler handler, http_configuration cfg)
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
    m_keep_alive = false;
    auto it = req.headers.find("Connection");
    if (it != req.headers.end())
      m_keep_alive = utils::to_lower(it->second) != "close";

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

  /// @brief Hook: a protocol-level violation was detected by the base state machine
  ///        (oversized request headers or body). Reply with the corresponding HTTP
  ///        error response (431 / 413) and force the connection to close after write.
  void on_protocol_error(status_code code, std::string_view what)
  {
    m_keep_alive = false; // force shutdown in on_write_complete
    auto resp = response::text(code, std::string(what));
    resp.headers["Connection"] = "close";
    do_write(resp);
  }
};

// ---------------------------------------------------------------------------
// http::server
// ---------------------------------------------------------------------------

server::server(asio::io_context &ctx, const std::uint16_t port, http_configuration cfg)
    : m_acceptor(ctx, tcp::endpoint(asio::ip::make_address("0.0.0.0"), port))
    , m_cfg(std::move(cfg))
    , m_waiter(details::create_default_exponential_backoff_waiter(ctx))
{
  m_default_handler = [](const request &) { return response::text(status_code::NOT_FOUND, "Not Found"); };
  do_accept();
}

server::~server() noexcept
{
  (void)m_waiter->cancel(); // TODO: logging number of canceled operations
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

    if (ec)
    {
      if (is_fatal_accept_error(ec)) // TODO: log fatal accept error
        return;
      else if (is_resource_exhaustion(ec))
      {
        // wait some time with backoff logic and try accept another one connection
        m_waiter->wait([this](const boost::system::error_code ec, const bool is_last_try) {
          // case where timer is broken
          if (ec)
            return;

          if (!is_last_try)
            do_accept();
        });
      }
      // skip another errors: connection_aborted, connection_reset, etc...
    }
    else
    {
      (void)m_waiter->reset(); // TODO: logging number of canceled operations
      auto handler = [this](const request &req) { return dispatch(req); };
      server_session::create(std::move(socket), handler, m_cfg)->start();
      do_accept();
    }
  });
}

bool server::is_fatal_accept_error(const boost::system::error_code &ec) const noexcept
{
  // clang-format off
  return ec == asio::error::bad_descriptor               ||
         ec == asio::error::access_denied                ||
         ec == asio::error::address_family_not_supported ||
         ec == asio::error::operation_not_supported      ||
         ec == asio::error::fault;
  // clang-format on
}

bool server::is_resource_exhaustion(const boost::system::error_code &ec) const noexcept
{
  return ec == asio::error::no_descriptors; // EMFILE
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
