#include "http/server.hpp"

#include <core/errors.hpp>
#include <http/session.hpp>

namespace http
{

// ---------------------------------------------------------------------------
// http::server_session
// ---------------------------------------------------------------------------

class server_session final : public session<server_session, request, response>
{
public:
  using read_t = request;
  using write_t = response;
  using pointer = std::shared_ptr<server_session>;

  [[nodiscard]] static pointer create(tcp::socket socket, request_handler handler, http_configuration cfg = {})
  {
    return std::shared_ptr<server_session>(new server_session(std::move(socket), std::move(handler), std::move(cfg)));
  }

  void start()
  {
    do_read();
  }

  // --- hooks from http_session ---

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

  void on_write_complete()
  {
    if (m_keep_alive)
      do_read();
    else
      shutdown();
  }

  void on_error(boost::system::error_code)
  {
    shutdown();
  }

private:
  server_session(tcp::socket socket, request_handler handler, http_configuration cfg)
      : session(std::move(socket), std::move(cfg)), m_handler(std::move(handler))
  {
  }

  request_handler m_handler;
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

std::size_t server::route_key_hash::operator()(const route_key &key) const
{
  auto h1 = std::hash<int> {}(static_cast<int>(key.method_type));
  auto h2 = std::hash<std::string> {}(key.path);
  return h1 ^ (h2 << 1);
}

} // namespace http
