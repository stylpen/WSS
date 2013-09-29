/*
 * Copyright (c) 2012, Stephan Wypler. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WSS Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL STEPHAN WYPLER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <websocketpp/sockets/tls.hpp>
#include <websocketpp/websocketpp.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <sstream>
#include <iomanip>

#define VERSION "MQTT VERSION BUILD 0.5"

// The Connection created on construction a new TCP connection.
// It forwards incoming TCP traffic to the websocket. That happens MQTT aware
// Its send() method sends stuff (from the websocket) to the TCP endpoint

template <typename endpoint_type> // plain or tls
class Connection : public boost::enable_shared_from_this<Connection>{
public:
//	typedef Connection<endpoint_type> me;
	typedef typename endpoint_type::handler::connection_ptr connection_ptr;
	Connection(connection_ptr con, boost::asio::io_service &io_service) :
		websocket_connection(con), socket(io_service), readBuffer(1024), mqttMessage(""){
#ifdef DEBUG
		std::cout << "In Connection Constructor: creating new connection object" << std::endl;
#endif
		boost::asio::ip::tcp::resolver resolver(io_service);
		boost::asio::ip::tcp::resolver::query query(Connection::hostname, Connection::port);
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end;
		boost::system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end) {
			socket.close();
			socket.connect(*endpoint_iterator++, error);
		}
		if (error) {
#ifdef DEBUG
			std::cerr << "connection error" << std::endl;
#endif
			websocket_connection->close(websocketpp::close::status::NORMAL, "can't establish tcp connection");
		}
#ifdef DEBUG
		std::cout << "Created new Connection at " << this << std::endl;
#endif
	}

	~Connection(){
#ifdef DEBUG
		std::cerr << "Beginning of Destructor of Connection" << std::endl << "   Connection address is: " << this << " websocket: " << websocket_connection << std::endl;
#endif
		websocket_connection->close(websocketpp::close::status::NORMAL, "closing");
#ifdef DEBUG
		std::cout << "End of Destructor of Connection" << std::endl;
#endif
	}

	/*
	 * processes the fixed 2 byte header. One of the following (and errors of course) can happen:
	 * 1) the remaining length is 0 and we just forward the 2 bytes
	 * 2) the remaining length is < 128 and we need to read as many bytes as the second byte tells us. receive_mqtt_message will do that and handle further processing.
	 * 3) the remaining length is < 128 (has a continuation bit set) and we need to figure out how long it will be and process it afterwards. receive_remaining_length will care about it.
	 * the method initializes the string mqttMessage (which is eventually sent via websocket) with the fixed header.
	 */
	void receive_header(const boost::system::error_code& error, size_t bytes_transferred){
		if (!error && bytes_transferred == 2 && websocket_connection->get_state() == websocketpp::session::state::OPEN){
#ifdef DEBUG
			std::cout << "received header from broker." << std::endl
					<< "Length: " << bytes_transferred << " Bytes" << std::endl
					<< std::setw(2) << std::setfill('0') << std::hex
					<< "Payload: " << (short)mqtt_header[0] << " " << (short)mqtt_header[1] << std::endl << std::endl << std::dec;
#endif
			mqttMessage = std::string(mqtt_header, bytes_transferred);
			if(mqtt_header[1] == 0){ // nothing following remaining length is 0 maybe that was a ping or something.
#ifdef DEBUG
			std::cout << "there is nothing more -> sending it to the websocket." << std::endl << std::endl;
#endif
			websocket_connection->send(mqttMessage,	websocketpp::frame::opcode::BINARY);
			// wait for new message
			start();
			} else if(mqtt_header[1] & 0x80){ // the length was not 0 and we need to read more remaining length bytes. receive_remaining_length will do it ...
#ifdef DEBUG
				std::cout << "need to read extra bytes for remaining length field" << std::endl << std::endl;
#endif
				multiplier = 1;
				remaining_length = mqtt_header[1] & 0x7F;
#ifdef DEBUG
				std::cout << "length initially is: " << remaining_length << std::endl;
#endif
				boost::asio::async_read(
						socket,
						boost::asio::buffer(&next_byte, 1),
						boost::asio::transfer_at_least(1),
						boost::bind(
								&Connection::receive_remaining_length,
								shared_from_this(),
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred
						)
				);
			} else{ // receive_mqtt_message will do the rest
				remaining_length = mqtt_header[1];
#ifdef DEBUG
				std::cout << "the remaining message length is: " << remaining_length << std::endl << std::endl;
#endif
				readBuffer.resize(remaining_length);
				boost::asio::async_read(
						socket,
						boost::asio::buffer(readBuffer, remaining_length),
						boost::asio::transfer_at_least(remaining_length),
						boost::bind(
								&Connection::receive_mqtt_message,
								shared_from_this(),
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred
						)
				);
			}
		} else {
#ifdef DEBUG
			std::cout << "stopped receiving header " << std::endl << bytes_transferred << " bytes arrived so far. error " << error << " websocket: " << websocket_connection << " connection " << this << std::endl << std::endl;
#endif
			if(error && error != boost::asio::error::operation_aborted)
				websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem while reading MQTT header");
		}
	}


	/*
	 * processes the remaining length field. One of the following (and errors of course) can happen:
	 * 1) this was the last byte and we can stop here and continue processing in receive_mqtt_message
	 * 2) there was again a continuation bit and we continue in that method
	 * the method appends the byte it reads to the string mqttMessage (which is eventually sent via websocket)
	 * algorithm for remaining length according to spec:
	 * multiplier = 1
	 * value = 0
	 * do
	 * 	digit = 'next digit from stream'
	 * 	value += (digit AND 127) * multiplier
	 * 	multiplier *= 128
	 * while ((digit AND 128) != 0)
	 */
	void receive_remaining_length(const boost::system::error_code& error, size_t bytes_transferred){
			if (!error && bytes_transferred == 1 && websocket_connection->get_state() == websocketpp::session::state::OPEN) {
#ifdef DEBUG
			std::cout << "received one more byte with remaining length: "
					<< std::setw(2) << std::setfill('0') << std::hex
					<< (short)next_byte << std::endl << std::dec;
#endif
				mqttMessage.append(std::string(&next_byte, 1));
#ifdef DEBUG
				std::cout << "there is still work to do" << mqtt_header[1] << std::endl << std::endl;
#endif
				multiplier *= 128;
				remaining_length += (next_byte & 0x7F) * multiplier;
#ifdef DEBUG
				std::cout << "remaining length is now: " << remaining_length << std::endl;
#endif
				if(next_byte & 0x80){ // there is still a continuation bit and we need to go on by calling this method again.
					boost::asio::async_read(
							socket,
							boost::asio::buffer(&next_byte, 1),
							boost::asio::transfer_at_least(1),
							boost::bind(
									&Connection::receive_remaining_length,
									shared_from_this(),
									boost::asio::placeholders::error,
									boost::asio::placeholders::bytes_transferred
							)
					);
				} else { // this was the last one and we now know the length of the remaining message. receive_mqtt_message will now do the rest
#ifdef DEBUG
					std::cout << "length finally is: "  << remaining_length << std::endl;
#endif
					readBuffer.resize(remaining_length);
					boost::asio::async_read(
							socket,
							boost::asio::buffer(readBuffer, remaining_length),
							boost::asio::transfer_at_least(remaining_length),
							boost::bind(
									&Connection::receive_mqtt_message,
									shared_from_this(),
									boost::asio::placeholders::error,
									boost::asio::placeholders::bytes_transferred
							)
					);
				}
			} else {
#ifdef DEBUG
				std::cout << "error reading length bytes " << std::endl << bytes_transferred << "bytes arrived so far " << error << " websocket: " << websocket_connection << " connection " << this << std::endl << std::endl;
#endif
				if(error && error != boost::asio::error::operation_aborted)
					websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem while reading remaining length");
			}
		}


	/*
	 * Gets called when remaining_length bytes were read from the socket.
	 * Appends the stuff to the mqtt_message string and sends it via websocket
	 */
	void receive_mqtt_message(const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error && websocket_connection->get_state() == websocketpp::session::state::OPEN){
#ifdef DEBUG
			std::cout << "received message body from broker " << bytes_transferred << " bytes: ";
			for(std::vector<char>::iterator it = readBuffer.begin(); it != readBuffer.end(); it++)
				std::cout << std::setw(2) << std::setfill('0') << std::hex << (short)*it << " ";
			std::cout << std::dec << std::endl;
		    std::cout << "plaintext: " << std::string(readBuffer.data(), bytes_transferred) << std::endl;
#endif
			mqttMessage.append(std::string(readBuffer.data(), bytes_transferred));
			websocket_connection->send(mqttMessage, websocketpp::frame::opcode::BINARY);
			// wait for new message
			start();
		} else {
#ifdef DEBUG
			std::cout << "error reading mqtt message (or operation was canceled)" << std::endl << bytes_transferred << "bytes arrived so far " << error << " websocket: " << websocket_connection << " connection " << this << std::endl << std::endl;
#endif
			if(error && error != boost::asio::error::operation_aborted)
				websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem in receice_message");
		}
	}

	void send(const std::string &message) {
#ifdef DEBUG
		std::cout << "sent TCP segment to broker " << message.size() << " bytes: " ;
		unsigned int i = 0;
		for(std::string::const_iterator it = message.begin(); it != message.end() && i <= message.size(); it++, i++)
			std::cout << std::setw(2) << std::setfill('0') << std::hex << (short)*it << " ";
		std::cout << std::dec << std::endl;
		std::cout << "plaintext: " << message << std::endl;
#endif
		try{
			socket.write_some(boost::asio::buffer(message.c_str(), message.size()));
		}catch(boost::system::system_error &e){
			std::cerr << "Write Error in TCP connection to broker: " << e.what() << std::endl;
			if(websocket_connection)
				websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem while sending");
		}
	}

	void start() {
#ifdef DEBUG
		std::cout << "Begin of start() in Connection at: " << this << std::endl;
#endif
		boost::asio::async_read(
				socket,
				boost::asio::buffer(mqtt_header, 2),
				boost::asio::transfer_at_least(2),
				boost::bind(
						&Connection::receive_header,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
				)
		);
#ifdef DEBUG
		std::cout << "  started async TCP read" << std::endl;
#endif
	}

	void stop(){
#ifdef DEBUG
		std::cerr << "Beginning of stop() in Connection at " << this << std::endl;
#endif
		socket.cancel();
		socket.close();
#ifdef DEBUG
		std::cout << "   stopped async TCP receive" << std::endl;
#endif
	}

	static std::string hostname;
	static std::string port;

private:
	websocketpp::server::handler::connection_ptr websocket_connection;
	boost::asio::ip::tcp::socket socket;
	std::vector<char> readBuffer;
	char mqtt_header[2];
	char next_byte;
	unsigned int remaining_length;
	std::string mqttMessage;
	unsigned int multiplier;
};

std::string Connection::hostname;
std::string Connection::port;

// The WebSocket++ handler 
template <typename endpoint_type> // plain or tls
class ServerHandler: public endpoint_type::handler {
public:
	typedef ServerHandler<endpoint_type> me;
	typedef typename endpoint_type::handler::connection_ptr connection_ptr;
	typedef typename endpoint_type::handler::message_ptr message_ptr;

	void on_open(connection_ptr con){
#ifdef DEBUG
		std::cerr << "In on_open" << std::endl << "   WEBSOCKET CONNECTION POINTER address is: " << con << std::endl;
#endif
		if (connections.find(con) == connections.end()) {
#ifdef DEBUG
			std::cout << "new connection, create new Connection handler to process this message" << std::endl;
#endif
			connections[con] = boost::shared_ptr<Connection>(new Connection(con, con->get_io_service()));
			connections[con]->start();
#ifdef DEBUG
		std::cerr << "Added new Connection to map" << std::endl << "   Connection address is: " << connections[con] << std::endl;
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
		typename std::map<connection_ptr, boost::shared_ptr<Connection> >::iterator it = connections.find(con);
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
			std::cerr << "End of on_close of Websocket" << std::endl << "  Removed Connection from map. Remaining Connections in map: " << connections.size() << std::endl << std::endl;
#endif
		}
#ifdef DEBUG
		else
			std::cerr << "   there was no connection to close. strange ..!" << std::endl;
#endif
	}

	void on_fail(connection_ptr con) {
#ifdef DEBUG
		std::cerr << "Start of on_fail() of Websocket. Try to delete Connection" << std::endl;
#endif
		typename std::map<connection_ptr, boost::shared_ptr<Connection> >::iterator it = connections.find(con);
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
			std::cerr << "End of on_fail of Websocket" << std::endl << "  Removed Connection from map. Remaining Connections in map: " << connections.size() << std::endl << std::endl;
#endif
		}
#ifdef DEBUG
		else
			std::cerr << "   failing! wasn't able to clean anything up." << std::endl;
#endif
	}

	std::string get_password() const {
	        return "test";
	    }

	boost::shared_ptr<boost::asio::ssl::context> on_tls_init() {
		// create a tls context, init, and return.
		boost::shared_ptr<boost::asio::ssl::context> context(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));
		try {
			context->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);
			context->set_password_callback(boost::bind(&me::get_password, this));
			context->use_certificate_chain_file(certChain);
			context->use_private_key_file(keyFile, boost::asio::ssl::context::pem);
			context->use_tmp_dh_file(dhFile);
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

private:
	typename std::map<connection_ptr, boost::shared_ptr<Connection> > connections;
};

template <typename endpoint_type> // plain or tls
std::string ServerHandler<endpoint_type>::keyFile;
template <typename endpoint_type> // plain or tls
std::string ServerHandler<endpoint_type>::certChain;
template <typename endpoint_type> // plain or tls
std::string ServerHandler<endpoint_type>::dhFile;

int main(int argc, char* argv[]){
	boost::program_options::options_description description("Available options");
	description.add_options()
		("help", "produce this help message")
		("websocketPort", boost::program_options::value<unsigned short>(), "specify the port where the websocket server should listen.\nDefault is 18883")
		("brokerHost", boost::program_options::value<std::string>(), "specify the host of the MQTT broker.\nDefault is localhost")
		("brokerPort", boost::program_options::value<std::string>(), "specify the port where the MQTT broker listens.\nDefault is 1883")
		("ws-keyfile", boost::program_options::value<std::string>(), "server key file for websocket side")
		("ws-chainfile", boost::program_options::value<std::string>(), "keychain for the websocket server key")
		("ws-dh-file", boost::program_options::value<std::string>(), "diffie- hellman parameter for websocket tls connection")
		("broker-certfile", boost::program_options::value<std::string>(), "certificate to use connection to MQTT broker")
		("tls-version", boost::program_options::value<std::string>(), "TLS version for connection to MQTT broker")
		("version", "print version number and exit")
		("verbose", "print websocket error messages");

	boost::program_options::variables_map variables_map;
	try {
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), variables_map);
		boost::program_options::notify(variables_map);
		if (variables_map.find("help") != variables_map.end()) {
			std::cout << description << "\n";
			return 1;
		}
		if (variables_map.find("version") != variables_map.end()) {
			std::cout << VERSION << "\n";
			return 0;
		}

		unsigned short websocketPort = variables_map.find("websocketPort") != variables_map.end() ? variables_map["websocketPort"].as<unsigned short>() : 18883;
		Connection::hostname = variables_map.find("brokerHost") != variables_map.end() ? variables_map["brokerHost"].as<std::string>() : "localhost";
		Connection::port = variables_map.find("brokerPort") != variables_map.end() ? variables_map["brokerPort"].as<std::string>() : "1883";


		if(variables_map.find("tls-version") != variables_map.end()){
			ServerHandler<websocketpp::server_tls>::keyFile = variables_map.find("ws-keyfile") != variables_map.end() ? variables_map["ws-keyfile"].as<std::string>() : "";
			ServerHandler<websocketpp::server_tls>::certChain = variables_map.find("ws-chainfile") != variables_map.end() ? variables_map["ws-chainfile"].as<std::string>() : "";
			ServerHandler<websocketpp::server_tls>::dhFile = variables_map.find("ws-dh-file") != variables_map.end() ? variables_map["ws-dh-file"].as<std::string>() : "";

			websocketpp::server_tls::handler::ptr serverHandler(new ServerHandler<websocketpp::server_tls>());
			websocketpp::server_tls websocketServer(serverHandler);

			boost::asio::signal_set signals(websocketServer.get_io_service(), SIGINT, SIGTERM);
			signals.async_wait(boost::bind(&websocketpp::server_tls::stop, boost::ref(websocketServer), true,  websocketpp::close::status::NORMAL, "websocket server quit"));

			websocketServer.alog().unset_level(websocketpp::log::alevel::ALL);
			websocketServer.elog().unset_level(websocketpp::log::elevel::ALL);
			if (variables_map.find("verbose") != variables_map.end()) {
				websocketServer.elog().set_level(websocketpp::log::elevel::RERROR);
				websocketServer.elog().set_level(websocketpp::log::elevel::FATAL);
			}
			websocketServer.listen(boost::asio::ip::tcp::v4(), websocketPort, 1);
		}else{

		}


	} catch(boost::program_options::unknown_option & e){
		std::cerr << e.what() << std::endl;
		std::cout << description << "\n";
		return 1;
	} catch(boost::system::system_error &e){
		std::cerr << "System Error: " << e.what() << std::endl;
		return 1;
	} catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
