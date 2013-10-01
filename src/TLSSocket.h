#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Socket.h"

#ifndef TLSSOCKET_H_
#define TLSSOCKET_H_

class TLS_Socket: public boost::enable_shared_from_this<TLS_Socket>, public Socket, private boost::asio::ssl::stream<boost::asio::ip::tcp::socket> {
public:
	TLS_Socket(boost::asio::io_service&, boost::asio::ssl::context&);
	void do_connect();

private:
    boost::asio::ip::tcp::socket &getSocketForAsio();
    bool verify_certificate(bool, boost::asio::ssl::verify_context&);
    void handle_connect(const boost::system::error_code&);
    void handle_handshake(const boost::system::error_code&);
};

#endif /* TLSSOCKET_H_ */
