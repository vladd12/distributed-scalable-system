#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/noncopyable.hpp>

namespace ipc
{

class async_service : public boost::noncopyable
{
protected:
  boost::asio::io_context &io_ctx;

private:
  void signals_handler()
  {
    io_ctx.stop();
  }

public:
  explicit async_service(boost::asio::io_context &ctx) : io_ctx(ctx)
  {
    boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(&async_service::signals_handler, this));
  }
};

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
  typedef std::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_context &io_context)
  {
    return std::make_shared<tcp_connection>(io_context);
  }

  boost::asio::ip::tcp::socket &socket()
  {
    return socket_;
  }

  void start()
  {
    // message_ = make_daytime_string();
    // boost::asio::ip::tcp::iostream stream;
    // stream.connect();

    boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(), boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
  }

private:
  tcp_connection(boost::asio::io_context &io_context) : socket_(io_context)
  {
  }

  void handle_write(const boost::system::error_code & /*error*/, size_t /*bytes_transferred*/)
  {
  }

  boost::asio::ip::tcp::socket socket_;
  std::string message_;
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

} // namespace ipc
