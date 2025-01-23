#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/noncopyable.hpp>

/*<!--- Separate, please ---!>*/
#include <config/configuration_v1.hpp>

namespace ipc
{

class async_service : public boost::noncopyable
{
protected:
  boost::asio::io_context &m_ctx;

public:
  explicit async_service(boost::asio::io_context &ctx) : m_ctx(ctx)
  {
    boost::asio::signal_set signals(m_ctx, SIGINT, SIGTERM);
    signals.async_wait([this](auto, auto) { m_ctx.stop(); });
  }
};

// class tcp_connection : public std::enable_shared_from_this<tcp_connection>
//{
// public:
//  typedef std::shared_ptr<tcp_connection> pointer;

//  static pointer create(boost::asio::io_context &io_context)
//  {
//    return std::make_shared<tcp_connection>(io_context);
//  }

//  boost::asio::ip::tcp::socket &socket()
//  {
//    return socket_;
//  }

//  void start()
//  {
//    // message_ = make_daytime_string();
//    // boost::asio::ip::tcp::iostream stream;
//    // stream.connect();

//    boost::asio::async_write(socket_, boost::asio::buffer(message_),
//        boost::bind(&tcp_connection::handle_write, shared_from_this(), boost::asio::placeholders::error,
//            boost::asio::placeholders::bytes_transferred));
//  }

// private:
//  tcp_connection(boost::asio::io_context &io_context) : socket_(io_context)
//  {
//  }

//  void handle_write(const boost::system::error_code & /*error*/, size_t /*bytes_transferred*/)
//  {
//  }

//  boost::asio::ip::tcp::socket socket_;
//  std::string message_;
//};

namespace asio = boost::asio;

constexpr std::size_t data_size = 2048;

/*
asio::awaitable<void> read()
{
  asio::ip::udp::socket udp_socket(io_context, asio::ip::udp::v4());
  {
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), source.port);
    auto address = asio::ip::address_v4::from_string(source.ip_address);
    auto interface = source.interface.empty() ? asio::ip::address_v4::any()
                                              : asio::ip::address_v4::from_string(source.interface);
    if (address.is_multicast())
    {
      auto group = asio::ip::multicast::join_group(address, interface);
      udp_socket.set_option(group);
    }
    udp_socket.set_option(asio::socket_base::reuse_address(true));
    udp_socket.bind(endpoint);
  }
  std::array<std::byte, data_size> data;
  asio::mutable_buffer buffer(data.data(), data.size());
  while (!io_context.stopped())
  {
    std::size_t received = co_await udp_socket.async_receive(buffer, asio::use_awaitable);
    callback(data.data(), received);
  }
  co_return;
}

class tcp_server : public async_service
{
private:
  std::uint16_t m_port;

  asio::awaitable<void> listener()
  {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, { asio::ip::tcp::v4(), m_port });
    while (!m_ctx.stopped())
    {
      asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
      asio::co_spawn(executor, reader(std::move(socket)), asio::detached);
    }
    co_return;
  }

  asio::awaitable<void> reader(asio::ip::tcp::socket socket)
  {
    std::array<std::byte, data_size> data;
    asio::mutable_buffer buffer(data.data(), data.size());
    while (!m_ctx.stopped())
    {
      std::size_t received = co_await socket.async_receive(buffer, asio::use_awaitable);
      read_handle(data.data(), received);
    }
    co_return;
  }

protected:
  virtual void read_handle(const std::byte *data, const std::size_t size) = 0;

public:
  explicit tcp_server(const std::uint16_t port, boost::asio::io_context &ctx) : async_service(ctx), m_port(port)
  {
    asio::co_spawn(m_ctx, listener(), asio::detached);
  }
};

class network_service : public async_service
{
};

class base_node_service : public network_service
{
};

class data_node_service : public base_node_service
{
};

class name_node_service : public data_node_service
{
};
*/

} // namespace ipc
