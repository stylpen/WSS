#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Socket.h"

#ifndef PLAINSOCKET_H_
#define PLAINSOCKET_H_

class Plain_Socket: public boost::enable_shared_from_this<Plain_Socket>, public Socket, public boost::asio::buffered_stream<boost::asio::ip::tcp::socket> {
public:
	Plain_Socket(boost::asio::io_service&);
	~Plain_Socket();
	void do_connect();
	void end();
	void async_read(boost::asio::mutable_buffers_1, std::size_t, boost::function<void(const boost::system::error_code&, size_t)>);
	void async_write(SharedBuffer, boost::function<void(const boost::system::error_code&, size_t)>);
private:
    boost::asio::ip::tcp::socket &getSocketForAsio();
	void handle_connect(const boost::system::error_code&);
};

#endif /* PLAINSOCKET_H_ */
