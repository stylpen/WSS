#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#ifndef SOCKET_H_
#define SOCKET_H_

class Socket{
public:
	virtual boost::asio::ip::tcp::socket& getSocketForAsio() = 0;
	virtual ~Socket() = 0;
	static boost::shared_ptr<Socket> create(boost::asio::io_service&, boost::asio::ssl::context*);
	virtual void do_connect() = 0;
	boost::function<void()> on_success;
	boost::function<void()> on_fail;
	virtual void async_read(boost::asio::mutable_buffers_1, std::size_t, boost::function<void(const boost::system::error_code&, size_t)>) = 0;
	virtual void async_write(boost::asio::const_buffers_1, boost::function<void(const boost::system::error_code&, size_t)>) = 0;
};

#endif /* SOCKET_H_ */
