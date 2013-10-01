#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>

#ifndef SOCKET_H_
#define SOCKET_H_

class Socket {
public:
	virtual boost::asio::ip::tcp::socket& getSocketForAsio() = 0;
	virtual ~Socket() = 0;
	static boost::shared_ptr<Socket> create(boost::asio::io_service&, boost::asio::ssl::context*);
	size_t _read(void*, size_t);
	size_t _write(const void *, size_t);
	virtual void do_connect() = 0;
	static std::string hostname;
	static std::string port;
};

#endif /* SOCKET_H_ */
