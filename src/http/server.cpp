#include "http/server.hpp"

#include <core/errors.hpp>
#include <http/session.hpp>
#include <utils/strings.hpp>

namespace
{
/// @brief   Returns true if an `async_accept` error means the acceptor itself
///          is broken and retrying is pointless.
/// @details `bad_descriptor` - acceptor is closed/invalid;
///          `access_denied` - missing privileges (port < 1024 without root);
///          other codes signal unsupported address family, unsupported operation,
///          or an internal fault. For these the server should stop accepting rather than spin.
bool is_fatal_accept_error(const boost::system::error_code &ec) noexcept
{
  // clang-format off
  return ec == boost::asio::error::bad_descriptor               ||
         ec == boost::asio::error::access_denied                ||
         ec == boost::asio::error::address_family_not_supported ||
         ec == boost::asio::error::operation_not_supported      ||
         ec == boost::asio::error::fault;
  // clang-format on
}

/// @brief   Returns true if an `async_accept` error is caused by exhausted
///          process resources and a delayed retry may succeed.
/// @details Currently only `no_descriptors` (EMFILE) - the process hit its
///          open-files limit. The accept loop backs off and retries; the
///          situation usually resolves once other sessions are closed.
bool is_resource_exhaustion(const boost::system::error_code &ec) noexcept
{
  return ec == boost::asio::error::no_descriptors; // EMFILE
}
}

namespace http
{

// ---------------------------------------------------------------------------
// http::server::server_session
// ---------------------------------------------------------------------------

/// @brief   One-shot HTTP server session: accepts a socket, reads a request,
///          dispatches it via the route handler, writes the response, then either
///          loops (keep-alive) or shuts down (Connection: close).
/// @details All HTTP state-machine logic lives in http::session. This class
///          only wires up: the request handler and the three hooks.
class server::server_session final : public session<server_session, request, response>
{
public:
  using shutdown_handler = std::function<void(const session_ptr &)>;

private:
  request_handler m_req_handler;  ///< User-provided request dispatcher
  shutdown_handler m_on_shutdown; ///< Shutdown callback.

  /// @brief   Private ctor of the server session object.
  /// @details Use create() for server session object creation.
  server_session(tcp::socket socket, request_handler req_handler, shutdown_handler on_shutdown, http_configuration cfg)
      : session(std::move(socket), std::move(cfg))
      , m_req_handler(std::move(req_handler))
      , m_on_shutdown(std::move(on_shutdown))
  {
  }

public:
  /// @brief Factory: wraps an accepted socket in a server_session.
  /// @param socket  Socket returned by acceptor (moved in).
  /// @param handler Request handler to dispatch to.
  /// @param cfg     Session configuration.
  [[nodiscard]] static pointer create(
      tcp::socket socket, request_handler req_handler, shutdown_handler on_shutdown, http_configuration cfg)
  {
    return std::shared_ptr<server_session>(
        new server_session(std::move(socket), std::move(req_handler), std::move(on_shutdown), std::move(cfg)));
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
      resp = m_req_handler(req);
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

  /// @brief   Hook: protocol-level violation detected by the base state machine
  ///          (malformed request, oversized headers/body, invalid framing).
  /// @details Sends the corresponding generic HTTP error response (status reason
  ///          phrase, no diagnostic details - those would leak parser internals
  ///          to the peer) and forces connection close. @p what is a diagnostic
  ///          message intended for logging only.
  void on_protocol_error(status_code code, std::string_view what)
  {
    // TODO: log at warn/error level with `what` and peer endpoint.
    (void)what;

    m_keep_alive = false;
    auto resp = response::text(code, std::string(status_text_for(code)));
    resp.headers["Connection"] = "close";
    do_write(resp);
  }

  /// @brief Called exactly once by base shutdown().
  void on_shutdown()
  {
    if (m_on_shutdown)
      m_on_shutdown(shared_from_this());
  }
};

// ---------------------------------------------------------------------------
// http::server
// ---------------------------------------------------------------------------

server::server(asio::io_context &ctx, const std::uint16_t port, http_configuration cfg)
    : m_ctx(ctx)
    , m_acceptor(ctx, tcp::endpoint(asio::ip::make_address("0.0.0.0"), port))
    , m_cfg(std::move(cfg))
    , m_waiter(details::create_default_exponential_backoff_waiter(ctx))
    , m_stopped(false)
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

void server::stop()
{
  {
    std::lock_guard<std::mutex> lock(m_sessions_mutex);
    if (m_stopped)
      return;
    m_stopped = true;
  }

  // Schedule the actual shutdown on the io_context thread to avoid racing
  // with in-flight session handlers (m_socket, m_timer, m_handler).
  asio::post(m_ctx, [this] {
    // 1) Stop accepting — cancels the pending async_accept.
    boost::system::error_code ignored;
    m_acceptor.close(ignored);

    // 2) Cancel the backoff waiter if it's mid-retry.
    if (m_waiter)
      (void)m_waiter->cancel();

    // 3) Shut down every live session.
    std::unordered_set<session_ptr> to_stop;
    {
      std::lock_guard<std::mutex> lock(m_sessions_mutex);
      to_stop = std::move(m_active_sessions);
      m_active_sessions.clear();
    }
    for (auto &s : to_stop)
      s->shutdown();
  });
}

std::size_t server::active_sessions() const
{
  std::lock_guard<std::mutex> lock(m_sessions_mutex);
  return m_active_sessions.size();
}

bool server::is_stopped() const
{
  std::lock_guard<std::mutex> lock(m_sessions_mutex);
  return m_stopped;
}

bool server::track_session(const session_ptr &session)
{
  std::lock_guard<std::mutex> lock(m_sessions_mutex);
  if (m_stopped)
    return false;
  m_active_sessions.insert(session);
  return true;
}

void server::untrack_session(const session_ptr &session)
{
  std::lock_guard<std::mutex> lock(m_sessions_mutex);
  m_active_sessions.erase(session);
}

void server::do_accept()
{
  m_acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
    if (ec == asio::error::operation_aborted)
      return;

    if (ec)
    {
      if (is_fatal_accept_error(ec))
      {
        // TODO: log fatal accept error
        stop();
        return;
      }
      else if (is_resource_exhaustion(ec))
      {
        // wait some time with backoff logic and try accept another one connection
        m_waiter->wait([this](const boost::system::error_code ec, const bool is_last_try) {
          // case where timer is broken
          if (ec)
            return;

          if (is_last_try)
          {
            // TODO: log exhausted retries.
            return;
          }
          do_accept();
        });
      }
      // Skip transient errors (connection_aborted, connection_reset, ...) - retry.
      do_accept();
    }
    else
    {
      (void)m_waiter->reset(); // TODO: logging number of canceled operations
      auto req_handler = [this](const request &req) { return dispatch(req); };
      auto shutdown_handler = [this](const session_ptr &s) { untrack_session(s); };
      auto session = server_session::create(std::move(socket), req_handler, shutdown_handler, m_cfg);

      if (!track_session(session))
      {
        // stop() raced with accept — discard the session.
        return;
      }

      session->start();
      do_accept();
    }
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
