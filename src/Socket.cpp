#include "Socket.h"
#include "PlainSocket.h"
#include "TLSSocket.h"

boost::shared_ptr<Socket> Socket::create(boost::asio::io_service& iIoService, boost::asio::ssl::context* ipSslContext) {
	if (ipSslContext == NULL) {
		return boost::shared_ptr<Socket>(new Plain_Socket(iIoService));
	}
   return boost::shared_ptr<Socket>(new TLS_Socket(iIoService, *ipSslContext));
}
 Socket::~Socket() {
	std::cout << "destructor of socket" << std::endl;
 };
