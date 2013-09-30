/*
 * TLSSocket.cpp
 *
 *  Created on: Sep 30, 2013
 *      Author: stephan
 */

#include "TLSSocket.h"

TLS_Socket::TLS_Socket(boost::asio::io_service& iIoService, boost::asio::ssl::context& iSslContext) :
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(iIoService, iSslContext) {
    }

boost::asio::ip::tcp::socket& TLS_Socket::getSocketForAsio() {
        return next_layer();
    }

