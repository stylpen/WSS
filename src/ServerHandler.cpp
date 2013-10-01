#include <websocketpp/sockets/tls.hpp>
#include <websocketpp/websocketpp.hpp>
#include "Connection.cpp"

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

	std::string get_password() const {
		return "test";
	}

	boost::shared_ptr<boost::asio::ssl::context> on_tls_init() {
		// create a tls context, init, and return.
		boost::shared_ptr<boost::asio::ssl::context> context(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1_server));
		try {
			context->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);
			context->set_password_callback(boost::bind(&me::get_password, this));
			context->use_certificate_chain_file(ServerHandler::certChain);
			context->use_private_key_file(ServerHandler::keyFile, boost::asio::ssl::context::pem);
			if(ServerHandler::dhFile != "")
				context->use_tmp_dh_file(ServerHandler::dhFile);
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		return context;
	}

	void http(connection_ptr con) {
		con->set_body("<!DOCTYPE html><html><head><title>WebSocket++ TLS certificate test</title></head><body><h1>WebSocket++ TLS certificate test</h1><p>This is an HTTP(S) page served by a WebSocket++ server for the purposes of confirming that certificates are working since browsers normally silently ignore certificate issues.</p></body></html>");
	}

	static std::string keyFile;
	static std::string certChain;
	static std::string dhFile;
	static std::string tlsVersion;

private:
	typename std::map<connection_ptr, boost::shared_ptr<Connection<endpoint_type> > > connections;
};

template <typename endpoint_type> // plain or tls
std::string ServerHandler<endpoint_type>::keyFile;
template <typename endpoint_type> // plain or tls
std::string ServerHandler<endpoint_type>::certChain;
template <typename endpoint_type> // plain or tls
std::string ServerHandler<endpoint_type>::dhFile;

#endif /* SERVER_HANDLER_H_ */
