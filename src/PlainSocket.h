#include <boost/asio.hpp>
#include "Socket.h"

#ifndef PLAINSOCKET_H_
#define PLAINSOCKET_H_

class Plain_Socket: public Socket, private boost::asio::buffered_stream<boost::asio::ip::tcp::socket> {
public:
	Plain_Socket(boost::asio::io_service&);

private:
    boost::asio::ip::tcp::socket &getSocketForAsio();
};

#endif /* PLAINSOCKET_H_ */
