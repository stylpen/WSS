#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Socket.h"

#ifndef PLAINSOCKET_H_
#define PLAINSOCKET_H_

class Plain_Socket: public boost::enable_shared_from_this<Plain_Socket>, public Socket, private boost::asio::buffered_stream<boost::asio::ip::tcp::socket> {
public:
	Plain_Socket(boost::asio::io_service&);
	void handle_connect(const boost::system::error_code&);
	void do_connect();

private:
    boost::asio::ip::tcp::socket &getSocketForAsio();
};

#endif /* PLAINSOCKET_H_ */
