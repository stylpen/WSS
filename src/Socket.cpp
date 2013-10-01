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

size_t Socket::_read(void *ipData, size_t iLength) {
	return boost::asio::read(getSocketForAsio(), boost::asio::buffer(ipData, iLength));
}
size_t Socket::_write(const void *ipData, size_t iLength) {
	return boost::asio::write(getSocketForAsio(), boost::asio::buffer(ipData, iLength));
}

std::string Socket::hostname;
std::string Socket::port;
