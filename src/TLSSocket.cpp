#include "TLSSocket.h"
#include "Options.h"
extern Options options;

TLS_Socket::TLS_Socket(boost::asio::io_service& iIoService, boost::asio::ssl::context& iSslContext) :
		 boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(iIoService, iSslContext) {
}

void TLS_Socket::async_read(boost::asio::mutable_buffers_1 buffer, std::size_t num, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_read(*(shared_from_this()), buffer, boost::asio::transfer_at_least(num), callback);
}

void TLS_Socket::async_write(boost::asio::const_buffers_1 buffer, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_write(*(shared_from_this()), buffer, callback);
}

void TLS_Socket::end(){
	async_shutdown(boost::bind(&TLS_Socket::handle_shutdown, shared_from_this(), boost::asio::placeholders::error));
}

void TLS_Socket::handle_shutdown(const boost::system::error_code& error){
	if(!error){
#ifdef DEBUG
		std::cerr << "TLS shutdown complete" << std::endl;
		get_io_service().post(on_end);
#endif
	} else {
#ifdef DEBUG
		std::cerr << "TLS shutdown failed: " << error.message() << std::endl;
#endif
		get_io_service().post(on_fail);
		get_io_service().post(on_end);
	}
}

void TLS_Socket::do_connect(){
	try{
		set_verify_mode(boost::asio::ssl::verify_none);
		set_verify_callback(boost::bind(&TLS_Socket::verify_certificate, this, _1, _2));
		boost::asio::ip::tcp::resolver resolver(get_io_service());
		boost::asio::ip::tcp::resolver::query query(options.broker_hostname, options.broker_port);
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
#ifdef DEBUG
		std::cerr << "will start async connect" << std::endl;
#endif
		boost::asio::async_connect(lowest_layer(), endpoint_iterator,
				boost::bind(&TLS_Socket::handle_connect, shared_from_this(),
						boost::asio::placeholders::error));
	}catch(std::exception &e){
#ifdef DEBUG
		std::cerr << "exception in async tls connect: " << e.what() << std::endl;
#endif
		get_io_service().post(on_fail);
	}
}

bool TLS_Socket::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx){
	// The verify callback can be used to check whether the certificate that is
	// being presented is valid for the peer. Note that the callback is called once
	// for each certificate in the certificate chain, starting from the root
	// certificate authority.
	// well, we could actually do something here some time ...
	return preverified;
}

void TLS_Socket::handle_connect(const boost::system::error_code& error){
#ifdef DEBUG
	std::cerr << "in handle connect of tls socket" << std::endl;
#endif
	if (!error){
#ifdef DEBUG
		std::cerr << "tls socket connected sucessfully. will start tls handshake" << std::endl;
#endif
		async_handshake(boost::asio::ssl::stream_base::client,
				boost::bind(&TLS_Socket::handle_handshake, shared_from_this(),
						boost::asio::placeholders::error));
	} else {
#ifdef DEBUG
		std::cerr << "Connect failed: " << error.message() << std::endl;
#endif
		get_io_service().post(on_fail);
	}
}

void TLS_Socket::handle_handshake(const boost::system::error_code& error){
	if (!error){
#ifdef DEBUG
		std::cout << "TLS Handshake successful:" << std::endl;
#endif
		get_io_service().post(on_success);
	} else {
#ifdef DEBUG
		std::cerr << "Handshake failed: " << error.message() << std::endl;
#endif
		get_io_service().post(on_fail);
	}
}

boost::asio::ip::tcp::socket& TLS_Socket::getSocketForAsio() {
	return next_layer();
}

TLS_Socket::~TLS_Socket(){
#ifdef DEBUG
		std::cerr << "Destructor of TLS Socket" << std::endl;
#endif
}
