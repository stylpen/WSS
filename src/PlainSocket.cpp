#include "PlainSocket.h"
#include "Connection.cpp"

Plain_Socket::Plain_Socket(boost::asio::io_service& iIoService, std::string hostname, std::string port) :
boost::asio::buffered_stream<boost::asio::ip::tcp::socket>(iIoService) {
	boost::asio::ip::tcp::resolver resolver(iIoService);
	boost::asio::ip::tcp::resolver::query query(hostname, port);
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
#ifdef DEBUG
	std::cerr << "will start async connect" << std::endl;
#endif
	boost::asio::async_connect(lowest_layer(), endpoint_iterator,
			boost::bind(&Plain_Socket::handle_connect, this,
					boost::asio::placeholders::error));

}

void Plain_Socket::handle_connect(const boost::system::error_code& error){
		if (!error){
#ifdef DEBUG
			std::cout << "Plain connection successful: " << std::endl;
#endif
		} else {
			std::cerr << "Connect failed: " << error.message() << std::endl;
		}
	}

boost::asio::ip::tcp::socket& Plain_Socket::getSocketForAsio() {
	return next_layer();
}
