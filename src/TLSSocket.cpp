#include "TLSSocket.h"
#include "Options.h"
extern Options options;

TLS_Socket::TLS_Socket(boost::asio::io_service& iIoService, boost::asio::ssl::context& iSslContext) :
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(iIoService, iSslContext) {
}

void TLS_Socket::async_read(boost::asio::mutable_buffers_1 buffer, std::size_t num, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_read(*(shared_from_this()), buffer, boost::asio::transfer_at_least(num), callback);
	std::cerr << "DOING MY ASYNC READ" << std::endl;
}

void TLS_Socket::async_write(boost::asio::const_buffers_1 buffer, boost::function<void(const boost::system::error_code&, size_t)> callback){
	boost::asio::async_write(*(shared_from_this()), buffer, callback);
	std::cerr << "DOING MY ASYNC WRITE" << std::endl;
}

void TLS_Socket::do_connect(){
	std::cerr << "in do connect of tls socket" << std::endl;
	try{
		set_verify_mode(boost::asio::ssl::verify_none);
		set_verify_callback(boost::bind(&TLS_Socket::verify_certificate, shared_from_this(), _1, _2));
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
		std::cerr << "exception in connect: " << e.what() << std::endl;
		on_fail();
#endif
	}
}

bool TLS_Socket::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx){
		// The verify callback can be used to check whether the certificate that is
		// being presented is valid for the peer. For example, RFC 2818 describes
		// the steps involved in doing this for HTTPS. Consult the OpenSSL
		// documentation for more details. Note that the callback is called once
		// for each certificate in the certificate chain, starting from the root
		// certificate authority.

		// In this example we will simply print the certificate's subject name.
		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		std::cout << "Verifying " << subject_name << std::endl;
		return preverified;
	}

void TLS_Socket::handle_connect(const boost::system::error_code& error){
		std::cerr << "in handle connect of tls socket" << std::endl;
		if (!error){
			async_handshake(boost::asio::ssl::stream_base::client,
					boost::bind(&TLS_Socket::handle_handshake, shared_from_this(),
							boost::asio::placeholders::error));
		} else {
			std::cerr << "Connect failed: " << error.message() << std::endl;
		}
	}

void TLS_Socket::handle_handshake(const boost::system::error_code& error){
		if (!error){
#ifdef DEBUG
			std::cout << "TLS Handshake successful:" << std::endl;
#endif
			on_success();
		} else {
			std::cerr << "Handshake failed: " << error.message() << std::endl;
			on_fail();
		}
	}

boost::asio::ip::tcp::socket& TLS_Socket::getSocketForAsio() {
        return next_layer();
    }
