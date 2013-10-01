#include "PlainSocket.h"

Plain_Socket::Plain_Socket(boost::asio::io_service& iIoService) :
boost::asio::buffered_stream<boost::asio::ip::tcp::socket>(iIoService) {
}

void Plain_Socket::do_connect(){
	try{
		boost::asio::ip::tcp::resolver resolver(get_io_service());
		boost::asio::ip::tcp::resolver::query query(Socket::hostname, Socket::port);
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	#ifdef DEBUG
		std::cerr << "will start async connect" << std::endl;
	#endif
		boost::asio::async_connect(lowest_layer(), endpoint_iterator,
				boost::bind(&Plain_Socket::handle_connect, shared_from_this(),
						boost::asio::placeholders::error));
	}catch(std::exception &e){
#ifdef DEBUG
		std::cerr << "exception in connect: " << e.what() << std::endl;
		on_fail();
#endif
	}
}

void Plain_Socket::handle_connect(const boost::system::error_code& error){
		if (!error){
#ifdef DEBUG
			std::cerr << "Plain connection successful: " << std::endl;
#endif
			on_success();
		} else {
			std::cerr << "Connect failed: " << error.message() << std::endl;
			on_fail();
		}
	}

boost::asio::ip::tcp::socket& Plain_Socket::getSocketForAsio() {
	return next_layer();
}

