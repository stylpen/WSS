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

#include <websocketpp/websocketpp.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <sstream>
#include <iomanip>

// The Connection created on construction a new TCP connection.
// It forwards incoming TCP traffic to the websocket. That happens MQTT aware
// Its send() method sends stuff (from the websocket) to the TCP endpoint

class Connection {
public:
	Connection(websocketpp::server::handler::connection_ptr con, boost::asio::io_service &io_service) :
		websocket_connection(con), socket(io_service), readBuffer(1024), mqttMessage(""){
#ifdef DEBUG
		std::cout << "creating connection object" << " connection " << this << std::endl;
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
			std::cerr << "connection error" << std::endl;
			websocket_connection->close(websocketpp::close::status::NORMAL, "cant establish tcp connection");
		} else {
#ifdef DEBUG
			std::cout << "Starting a TCP client" << std::endl;
#endif
			start();
		}
	}

	~Connection(){
		socket.cancel();
		websocket_connection->close(websocketpp::close::status::NORMAL, "closing");
#ifdef DEBUG
		std::cout << "destructor websocket: " << websocket_connection << " connection " << this << std::endl;
#endif
		stop();
	}

	/*
	 * processes the fixed 2 byte header. One of the following (and errors of course) can happen:
	 * 1) the remaining length is 0 and we just forward the 2 bytes
	 * 2) the remaining length is < 128 and we need to read as many bytes as the second byte tells us. receive_mqtt_message will do that and handle further processing.
	 * 3) the remaining length is < 128 (has a continuation bit set) and we need to figure out how long it will be and process it afterwards. receive_remaining_length will care about it.
	 * the method initializes the string mqttMessage (which is eventually sent via websocket) with the fixed header.
	 */
	void receive_header(const boost::system::error_code& error, size_t bytes_transferred){
		if (!error && bytes_transferred == 2){
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
			} else if(mqtt_header[1] & 0x80){ // the length was not 0 and we need to read more remaining length bytes. receive_remaining_length will do it ...
#ifdef DEBUG
				std::cout << "need to read extra bytes for remaining length field" << std::endl << std::endl;
#endif
				remaining_length = mqtt_header[1] & 0x7F;
				boost::asio::async_read(
						socket,
						boost::asio::buffer(&next_byte, 1),
						boost::asio::transfer_at_least(1),
						boost::bind(
								&Connection::receive_remaining_length,
								this,
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
								this,
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred
						)
				);
			}
		} else {
#ifdef DEBUG
			std::cout << "stopped receiving header " << std::endl << bytes_transferred << " bytes arrived so far. error " << error << " websocket: " << websocket_connection << " connection " << this << std::endl << std::endl;
#endif
			if(error != boost::asio::error::operation_aborted)
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
		static unsigned int multiplier = 1;
			if (!error && bytes_transferred == 1) {
#ifdef DEBUG
			std::cout << "received one more byte with remaining length: "
					<< std::setw(2) << std::setfill('0') << std::hex
					<< (short)next_byte << std::endl << std::dec;
#endif
				mqttMessage.append(std::string(&next_byte, 1));
				if(next_byte & 0x80){ // there is still a continuation bit and we need to go on by calling this method again.
#ifdef DEBUG
					std::cout << "there is still work to do" << mqtt_header[1] << std::endl << std::endl;
#endif
					remaining_length += (next_byte & 0x7F) * multiplier;
					multiplier *= 128;
					boost::asio::async_read(
							socket,
							boost::asio::buffer(&next_byte, 1),
							boost::asio::transfer_at_least(1),
							boost::bind(
									&Connection::receive_remaining_length,
									this,
									boost::asio::placeholders::error,
									boost::asio::placeholders::bytes_transferred
							)
					);
				} else { // this was the last one and we now know the length of the remaining message. receive_mqtt_message will now do the rest
					readBuffer.resize(remaining_length);
					multiplier = 1;
					boost::asio::async_read(
							socket,
							boost::asio::buffer(readBuffer, remaining_length),
							boost::asio::transfer_at_least(remaining_length),
							boost::bind(
									&Connection::receive_mqtt_message,
									this,
									boost::asio::placeholders::error,
									boost::asio::placeholders::bytes_transferred
							)
					);
				}
			} else {
#ifdef DEBUG
				std::cout << "error reading length bytes " << std::endl << bytes_transferred << "bytes arrived so far" << std::endl<< std::endl;
#endif
				if(error != boost::asio::error::operation_aborted)
					websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem while reading remaining length");
			}
		}


	/*
	 * Gets called when remaining_length bytes were read from the socket.
	 * Appends the stuff to the mqtt_message string and sends it via websocket
	 */
	void receive_mqtt_message(const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error && bytes_transferred == remaining_length){
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
		}else {
#ifdef DEBUG
			std::cout << "error reading mqtt message " << std::endl << bytes_transferred << "bytes arrived so far" << std::endl<< std::endl;
#endif
			if(error != boost::asio::error::operation_aborted)
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
		boost::asio::async_read(
				socket,
				boost::asio::buffer(mqtt_header, 2),
				boost::asio::transfer_at_least(2),
				boost::bind(
						&Connection::receive_header,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
				)
		);
#ifdef DEBUG
		std::cout << "started async TCP read" << std::endl;
#endif
	}

	void stop() {
		socket.close();
#ifdef DEBUG
		std::cout << "stopped async TCP receive" << std::endl;
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
};

std::string Connection::hostname;
std::string Connection::port;

// The WebSocket++ handler 
class ServerHandler: public websocketpp::server::handler {
public:
	void on_open(connection_ptr con) {
		if (connections.find(con) == connections.end()) {
#ifdef DEBUG
			std::cout << "new connection, create new handler to process this message" << std::endl;
#endif
			connections[con] = new Connection(con, con->get_io_service());
		} else
			std::cerr << "did I just reuse a connection pointer???" << std::endl;
	}

	void on_message(connection_ptr con, message_ptr msg) {
		if (connections.find(con) != connections.end()){
#ifdef DEBUG
            std::cout << "received from websocket: " << msg->get_payload() << std::endl;
#endif
			connections[con]->send(msg->get_payload());
		}else
			std::cerr << "failed receiving websocket message" << std::endl;
	}

	void on_close(connection_ptr con) {
		std::map<connection_ptr, Connection*>::iterator it = connections.find(con);
		if (it != connections.end()) {
			if(connections[con])
				delete connections[con];
			connections.erase(it);
#ifdef DEBUG
			std::cout << "closing connection and deleting corresponding handler. number of handlers left:" << connections.size() << std::endl;
#endif
		} else
			std::cerr << "there was no connection to close. strange ..!" << std::endl;
	}

	void on_fail(connection_ptr con) {
		std::map<connection_ptr, Connection*>::iterator it = connections.find(con);
		if (it != connections.end()) {
			if(connections[con])
				delete connections[con];
			connections.erase(it);
#ifdef DEBUG
			std::cout << "something really failed .. tried to clean up. number of handlers left:" << connections.size() << std::endl;
#endif
		} else
			std::cerr << "failing! wasn't able to clean anything up." << std::endl;
	}

private:
	std::map<connection_ptr, Connection*> connections;
};

int main(int argc, char* argv[]){
	boost::program_options::options_description description("Available options");
	description.add_options()
		("help", "produce this help message")
		("websocketPort", boost::program_options::value<unsigned short>(), "specify the port where the websocket server should listen.\nDefault is 1337")
		("brokerHost", boost::program_options::value<std::string>(), "specify the host of the MQTT broker.\nDefault is localhost")
		("brokerPort", boost::program_options::value<std::string>(), "specify the port where the MQTT broker listens.\nDefault is 1883");
	boost::program_options::variables_map variables_map;
	try {
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), variables_map);
		boost::program_options::notify(variables_map);
		if (variables_map.find("help") != variables_map.end()) {
			std::cout << description << "\n";
			return 1;
		}

		unsigned short websocketPort = variables_map.find("websocketPort") != variables_map.end() ? variables_map["websocketPort"].as<unsigned short>() : 1337;
		Connection::hostname = variables_map.find("brokerHost") != variables_map.end() ? variables_map["brokerHost"].as<std::string>() : "localhost";
		Connection::port = variables_map.find("brokerPort") != variables_map.end() ? variables_map["brokerPort"].as<std::string>() : "1883";

		websocketpp::server::handler::ptr serverHandler = websocketpp::server::handler::ptr(new ServerHandler());
		websocketpp::server websocketServer(serverHandler);

		boost::asio::signal_set signals(websocketServer.get_io_service(), SIGINT, SIGTERM);
		signals.async_wait(boost::bind(&websocketpp::server::stop, boost::ref(websocketServer), true,  websocketpp::close::status::NORMAL, "websocket server quit"));

		websocketServer.alog().unset_level(websocketpp::log::alevel::ALL);
		websocketServer.elog().unset_level(websocketpp::log::elevel::ALL);
		websocketServer.elog().set_level(websocketpp::log::elevel::RERROR);
		websocketServer.elog().set_level(websocketpp::log::elevel::FATAL);
		websocketServer.listen(boost::asio::ip::tcp::v4(), websocketPort, 1);
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
