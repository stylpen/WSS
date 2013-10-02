#include "PlainSocket.h"
#include "Options.h"
extern Options options;


Plain_Socket::Plain_Socket(boost::asio::io_service& iIoService) :
boost::asio::buffered_stream<boost::asio::ip::tcp::socket>(iIoService) {
}

void Plain_Socket::async_read(boost::asio::mutable_buffers_1 buffer, std::size_t num, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_read(next_layer(), buffer, boost::asio::transfer_at_least(num), callback);
	std::cerr << "DOING MY ASYNC READ" << std::endl;
}

void Plain_Socket::async_write(boost::asio::const_buffers_1 buffer, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_write(next_layer(), buffer, callback);
	std::cerr << "DOING MY ASYNC WRITE" << std::endl;
}

void Plain_Socket::do_connect(){
	try{
		boost::asio::ip::tcp::resolver resolver(get_io_service());
		boost::asio::ip::tcp::resolver::query query(options.broker_hostname, options.broker_port);
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

