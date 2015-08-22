#include "PlainSocket.h"
#include "Options.h"
extern Options options;


Plain_Socket::Plain_Socket(boost::asio::io_service& iIoService) :
		 boost::asio::buffered_stream<boost::asio::ip::tcp::socket>(iIoService){
}

void Plain_Socket::async_read(boost::asio::mutable_buffers_1 buffer, std::size_t num, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_read(next_layer(), buffer, boost::asio::transfer_at_least(num), callback);
}

void Plain_Socket::async_write(SharedBuffer buffer, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_write(next_layer(), buffer, callback);
}

void Plain_Socket::do_connect(){
	try{
		boost::asio::ip::tcp::resolver resolver(get_io_service());
		boost::asio::ip::tcp::resolver::query query(options.broker_hostname, options.broker_port, boost::asio::ip::resolver_query_base::numeric_service);
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
#ifdef DEBUG
		std::cerr << "will start plain async connect" << std::endl;
#endif
		boost::asio::async_connect(lowest_layer(), endpoint_iterator,
				boost::bind(&Plain_Socket::handle_connect, shared_from_this(),
						boost::asio::placeholders::error));
	}catch(std::exception &e){
		if(options.verbose)
			std::cerr << "Failed connecting to broker: " << e.what() << std::endl;
		get_io_service().post(on_fail);
	}
}

void Plain_Socket::handle_connect(const boost::system::error_code& error){
	if (!error){
#ifdef DEBUG
		std::cerr << "Plain connection successful: " << std::endl;
#endif
		get_io_service().post(on_success);
	} else {
		if(options.verbose)
			std::cerr << "Failed connecting to broker: " << error.message() << std::endl;
		get_io_service().post(on_fail);
	}
}

void Plain_Socket::end(){
	get_io_service().post(on_end);
}


boost::asio::ip::tcp::socket& Plain_Socket::getSocketForAsio() {
	return next_layer();
}

Plain_Socket::~Plain_Socket(){
#ifdef DEBUG
		std::cerr << "Destructor of Plain Socket" << std::endl;
#endif
}

