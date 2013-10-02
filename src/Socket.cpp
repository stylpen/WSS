#include "Socket.h"
#include "PlainSocket.h"
#include "TLSSocket.h"
#include "Options.h"
extern Options options;

boost::shared_ptr<Socket> Socket::create(boost::asio::io_service& iIoService) {
#ifdef DEBUG
	std::cerr << "In Socket::create()" << std::endl;
#endif
	if(options.broker_tls){
#ifdef DEBUG
		std::cerr << "options state that there should be a tls connection" << std::endl;
#endif
		boost::shared_ptr<boost::asio::ssl::context> ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_client));
		ctx->load_verify_file(options.broker_ca);
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
