/*
 * TLSSocket.cpp
 *
 *  Created on: Sep 30, 2013
 *      Author: stephan
 */

#include "TLSSocket.h"

TLS_Socket::TLS_Socket(boost::asio::io_service& iIoService, boost::asio::ssl::context& iSslContext, std::string hostname, std::string port) :
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(iIoService, iSslContext) {
	set_verify_mode(boost::asio::ssl::verify_none);
	set_verify_callback(boost::bind(&TLS_Socket::verify_certificate, this, _1, _2));
	boost::asio::ip::tcp::resolver resolver(iIoService);
	boost::asio::ip::tcp::resolver::query query(hostname, port);
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
#ifdef DEBUG
	std::cerr << "will start async connect" << std::endl;
#endif
	boost::asio::async_connect(lowest_layer(), endpoint_iterator,
	        boost::bind(&TLS_Socket::handle_connect, this,
	          boost::asio::placeholders::error));
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
		if (!error){
			async_handshake(boost::asio::ssl::stream_base::client,
					boost::bind(&TLS_Socket::handle_handshake, this,
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
		} else {
			std::cerr << "Handshake failed: " << error.message() << std::endl;
		}
	}

boost::asio::ip::tcp::socket& TLS_Socket::getSocketForAsio() {
        return next_layer();
    }

