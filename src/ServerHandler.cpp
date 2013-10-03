#include <websocketpp/sockets/tls.hpp>
#include <websocketpp/websocketpp.hpp>
#include "Connection.cpp"
#include "Options.h"

extern Options options;

#ifndef SERVER_HANDLER_H_
#define SERVER_HANDLER_H_

// The WebSocket++ handler
template <typename endpoint_type> // plain or tls
class ServerHandler: public endpoint_type::handler {
public:
	typedef ServerHandler<endpoint_type> me;
	typedef typename endpoint_type::handler::connection_ptr connection_ptr;
	typedef typename endpoint_type::handler::message_ptr message_ptr;

	void on_open(connection_ptr con){
#ifdef DEBUG
		std::cerr << "In on_open" << std::endl << " WEBSOCKET CONNECTION POINTER address is: " << con << std::endl;
#endif
		if (connections.find(con) == connections.end()) {
#ifdef DEBUG
			std::cerr << "new connection, create new Connection handler to process this message" << std::endl;
#endif
			connections[con] = boost::shared_ptr<Connection<endpoint_type> >(new Connection<endpoint_type>(con, con->get_io_service()));
			connections[con]->init();
#ifdef DEBUG
			std::cerr << "Added new Connection to map" << std::endl << " Connection address is: " << connections[con] << std::endl;
#endif
		}
#ifdef DEBUG
		else
			std::cerr << "did I just reuse a connection pointer???" << std::endl;
#endif
	}

	void on_message(connection_ptr con, message_ptr msg) {
		if (connections.find(con) != connections.end()){
#ifdef DEBUG
			std::cout << "received from websocket: " << msg->get_payload() << std::endl;
#endif
			connections[con]->send(msg->get_payload());
		}
#ifdef DEBUG
		else
			std::cerr << "failed receiving websocket message" << std::endl;
#endif
	}

	void on_close(connection_ptr con) {
#ifdef DEBUG
		std::cerr << "Start of on_close() of Websocket. Try to delete Connection" << std::endl;
#endif
		typename std::map<connection_ptr, boost::shared_ptr<Connection<endpoint_type> > >::iterator it = connections.find(con);
		if (it != connections.end()) {
#ifdef DEBUG
			std::cerr << "Will delete Connection if it exists" << std::endl;
#endif
			if(connections[con]){
					connections[con]->stop();
					connections[con].reset();
			}
			connections.erase(it);
#ifdef DEBUG
			std::cerr << "End of on_close of Websocket" << std::endl << " Removed Connection from map. Remaining Connections in map: " << connections.size() << std::endl << std::endl;
#endif
		}
#ifdef DEBUG
		else
			std::cerr << " there was no connection to close. strange ..!" << std::endl;
#endif
	}

	void on_fail(connection_ptr con) {
#ifdef DEBUG
		std::cerr << "Start of on_fail() of Websocket. Try to delete Connection" << std::endl;
#endif
		typename std::map<connection_ptr, boost::shared_ptr<Connection<endpoint_type> > >::iterator it = connections.find(con);
		if (it != connections.end()) {
#ifdef DEBUG
			std::cerr << "Will delete Connection if it exists" << std::endl;
#endif
			if(connections[con]){
				connections[con]->stop();
				connections[con].reset();
			}
			connections.erase(it);
#ifdef DEBUG
			std::cerr << "End of on_fail of Websocket" << std::endl << " Removed Connection from map. Remaining Connections in map: " << connections.size() << std::endl << std::endl;
#endif
		}
#ifdef DEBUG
		else
			std::cerr << " failing! wasn't able to clean anything up." << std::endl;
#endif
	}

private:
	typename std::map<connection_ptr, boost::shared_ptr<Connection<endpoint_type> > > connections;
};

class PlainServerHandler : public ServerHandler<websocketpp::server> {
};

class TLSServerHandler : public ServerHandler<websocketpp::server_tls> {

	typedef websocketpp::server_tls::handler::connection_ptr connection_ptr;
	typedef websocketpp::server_tls::handler::message_ptr message_ptr;

	// implement this if you want to use encrypted certificates
	std::string get_password() const {
		return "password";
	}

	boost::shared_ptr<boost::asio::ssl::context> on_tls_init() {
		boost::shared_ptr<boost::asio::ssl::context> context;
#if BOOST_VERSION < 105400
		context = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_server));
#ifdef DEBUG
		if(options.ws_tls_version != "TLSV1")
			std::cerr << "The version of boost WSS is compiled with allows only TLSv1. Using that version." << std::endl;
#endif
#else
		if(options.ws_tls_version == "TLSV1")
			context = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_server));
		else if(options.ws_tls_version == "TLSV11")
			context = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv11_server));
		else if(options.ws_tls_version == "TLSV12")
			context = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12_server));
		else{
#ifdef DEBUG
			std::cerr << "Unknown TLS version \"" << options.ws_tls_version << "\". Taking TLSv1 instead." << std::endl;
#endif
			context = boost::shared_ptr<boost::asio::ssl::context>(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_server));
		}
#endif
		try {
			context->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);
			context->set_password_callback(boost::bind(&TLSServerHandler::get_password, this));
			context->use_certificate_chain_file(options.ws_crt);
			context->use_private_key_file(options.ws_key, boost::asio::ssl::context::pem);
			if(options.ws_dh != "")
				context->use_tmp_dh_file(options.ws_dh);
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		return context;
	}

	void http(connection_ptr con) {
		con->set_body("<!DOCTYPE html><html><head><title>WSS TLS certificate test</title></head><body><h1>WSS TLS certificate test</h1><p>Secure WebSockets should work if you can see this.</p></body></html>");
	}
};

#endif /* SERVER_HANDLER_H_ */
