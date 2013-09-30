#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#ifndef SOCKET_H_
#define SOCKET_H_

class Socket {
public:
	virtual boost::asio::ip::tcp::socket& getSocketForAsio() = 0;
	virtual ~Socket() = 0;
	static Socket* create(boost::asio::io_service&, boost::asio::ssl::context*, std::string, std::string);
	size_t _read(void*, size_t);
	size_t _write(const void *, size_t);
};

#endif /* SOCKET_H_ */
