#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "Socket.h"

#ifndef TLSSOCKET_H_
#define TLSSOCKET_H_

class TLS_Socket: public Socket, private boost::asio::ssl::stream<boost::asio::ip::tcp::socket> {
public:
	TLS_Socket(boost::asio::io_service&, boost::asio::ssl::context&);

private:
    boost::asio::ip::tcp::socket &getSocketForAsio();
};

#endif /* TLSSOCKET_H_ */
