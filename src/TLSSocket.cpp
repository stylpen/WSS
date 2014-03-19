#include "TLSSocket.h"
#include "Options.h"
#include <boost/asio/ssl.hpp>
extern Options options;

TLS_Socket::TLS_Socket(boost::asio::io_service& iIoService, boost::asio::ssl::context& iSslContext) :
		 boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(iIoService, iSslContext) {
}

void TLS_Socket::async_read(boost::asio::mutable_buffers_1 buffer, std::size_t num, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_read(*(shared_from_this()), buffer, boost::asio::transfer_at_least(num), callback);
}

void TLS_Socket::async_write(SharedBuffer buffer, boost::function<void(const boost::system::error_code&, size_t)> callback){
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
		if(options.verbose)
			std::cerr << "TLS shutdown failed: " << error.message() << std::endl;
		get_io_service().post(on_fail);
		get_io_service().post(on_end);
	}
}

void TLS_Socket::do_connect(){
	try{
		if(options.broker_ca == "")
			set_verify_mode(boost::asio::ssl::verify_none);
		else
			set_verify_mode(boost::asio::ssl::verify_peer);
		set_verify_callback(boost::bind(&TLS_Socket::verify_certificate, this, _1, _2));
		boost::asio::ip::tcp::resolver resolver(get_io_service());
		boost::asio::ip::tcp::resolver::query query(options.broker_hostname, options.broker_port, boost::asio::ip::resolver_query_base::numeric_service);
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
#ifdef DEBUG
		std::cerr << "will start async connect" << std::endl;
#endif
		boost::asio::async_connect(lowest_layer(), endpoint_iterator,
				boost::bind(&TLS_Socket::handle_connect, shared_from_this(),
						boost::asio::placeholders::error));
	}catch(std::exception &e){
		if(options.verbose)
			std::cerr << "exception in async tls connect: " << e.what() << std::endl;
		get_io_service().post(on_fail);
	}
}

bool TLS_Socket::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx){
#ifdef DEBUG
	if(preverified == false)
		std::cerr << "preverification error: " << X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx.native_handle())) << std::endl;
#endif
	if((X509_STORE_CTX_get_error(ctx.native_handle()) == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN
		|| X509_STORE_CTX_get_error(ctx.native_handle()) == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT
	   ) && options.broker_allow_self_signed_certificates){
#ifdef DEBUG
		std::cerr << "accepting self signed certificate" << std::endl;
#endif
		preverified = true;
	}
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
		if(options.verbose)
			std::cerr << "Failed connecting to broker: " << error.message() << std::endl;
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
		if(options.verbose)
			std::cerr << "TLS handshake with broker failed: " << error.message() << std::endl;
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
