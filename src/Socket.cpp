#include "Socket.h"
#include "PlainSocket.h"
#include "TLSSocket.h"
#include "Options.h"
#include <boost/system/error_code.hpp>
extern Options options;

boost::shared_ptr<Socket> Socket::create(boost::asio::io_service& iIoService) {
#ifdef DEBUG
	std::cerr << "In Socket::create()" << std::endl;
#endif
	if(options.broker_tls){
#ifdef DEBUG
		std::cerr << "options state that there should be a tls connection" << std::endl;
#endif
		boost::shared_ptr<boost::asio::ssl::context> ctx;
#if BOOST_VERSION < 105400
		ctx = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_client));
		if(options.verbose && options.broker_tls_version != "TLSV1")
			std::cerr << "The version of boost WSS is compiled with allows only TLSv1. Using that version." << std::endl;
#else
		if(options.broker_tls_version == "TLSV1")
			ctx = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_client));
		else if(options.broker_tls_version == "TLSV11")
			ctx = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv11_client));
		else if(options.broker_tls_version == "TLSV12")
			ctx = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12_client));
		else{
			if(options.verbose)
				std::cerr << "Unknown TLS version \"" << options.broker_tls_version << "\". Taking TLSv1 instead." << std::endl;
			ctx = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_client));
		}
#endif
		if(options.broker_ca != ""){
			try{
				ctx->load_verify_file(options.broker_ca);
			}
			catch(std::exception &e){
				std::cerr << "Cannot load broker ca file: " << e.what() << std::endl;
			}
		}
		return boost::shared_ptr<Socket>(new TLS_Socket(iIoService, *ctx));
	} else{
#ifdef DEBUG
		std::cerr << "options state that there should be a plain tcp connection" << std::endl;
#endif
		return boost::shared_ptr<Socket>(new Plain_Socket(iIoService));
	}
}

 Socket::~Socket() {
 };
