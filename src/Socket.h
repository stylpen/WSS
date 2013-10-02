#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#ifndef SOCKET_H_
#define SOCKET_H_

class Socket {
public:
	virtual boost::asio::ip::tcp::socket& getSocketForAsio() = 0;
	virtual ~Socket() = 0;
	static boost::shared_ptr<Socket> create(boost::asio::io_service&, boost::asio::ssl::context*);
	virtual void do_connect() = 0;
	static std::string hostname;
	static std::string port;
	boost::function<void()> on_success;
	boost::function<void()> on_fail;
};

#endif /* SOCKET_H_ */
