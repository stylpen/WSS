#include "PlainSocket.h"

	Plain_Socket::Plain_Socket(boost::asio::io_service& iIoService) :
		boost::asio::buffered_stream<boost::asio::ip::tcp::socket>(iIoService) {
    }

    boost::asio::ip::tcp::socket& Plain_Socket::getSocketForAsio() {
        return next_layer();
    }
