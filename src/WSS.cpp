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
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstring>
#include <sstream>
#include <iomanip>

// The Connection created on construction a new TCP connection.
// It forwards incoming TCP traffic to the websocket.
// Its send() method sends stuff (from the websocket) to the TCP endpoint

class Connection {
public:
	Connection(websocketpp::server::handler::connection_ptr con, boost::asio::io_service &io_service) :
		websocket_connection(con), socket(io_service), readBuffer(1500){
#ifdef DEBUG
		std::cout << "creating connection object" << std::endl;
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
		stop();
	}

	void receive(const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error) {
			std::string message(readBuffer.data(), bytes_transferred);
#ifdef DEBUG
			std::cout << "received " << bytes_transferred << " bytes: ";
			for(std::vector<char>::iterator it = readBuffer.begin(); it != readBuffer.end(); it++){
				std::cout << std::setw(2) << std::setfill('0') << std::hex << (short)*it << " ";
			}
			std::cout << std::dec << std::endl;
#endif
			websocket_connection->send(message, websocketpp::frame::opcode::BINARY);
			socket.async_receive(
					boost::asio::buffer(readBuffer),
					boost::bind(&Connection::receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
#ifdef DEBUG
		else {
			std::cout << "I'm done" << std::endl;
		}
#endif
	}

	void send(const std::string &message) {
#ifdef DEBUG
		std::cout << "sent " << message.size() << " bytes: " ;
		for(std::string::const_iterator it = message.begin(); it != message.end(); it++)
			std::cout << std::setw(2) << std::setfill('0') << std::hex << (short)*it << " ";
		std::cout << std::dec << std::endl;
#endif
		try{
			socket.write_some(boost::asio::buffer(message.c_str(), message.size()));
		}catch(boost::system::system_error &e){
			std::cerr << "Write Error: " << e.what() << std::endl;
			websocket_connection->close(websocketpp::close::status::NORMAL, "cant establish tcp connection");
		}
	}

	void start() {
		socket.async_receive(
				boost::asio::buffer(readBuffer),
				boost::bind(&Connection::receive, this,	boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
#ifdef DEBUG
		std::cout << "started async receive" << std::endl;
#endif
	}

	void stop() {
		socket.close();
#ifdef DEBUG
		std::cout << "stopped async receive" << std::endl;
#endif
	}

	static std::string hostname;
	static std::string port;

private:
	websocketpp::server::handler::connection_ptr websocket_connection;
	boost::asio::ip::tcp::socket socket;
	std::vector<char> readBuffer;
};

std::string Connection::hostname;
std::string Connection::port;

// The WebSocket++ handler in this case reads numbers from connections and packs
// connection pointer + number into a request struct and passes it off to the
// coordinator.
class ServerHandler: public websocketpp::server::handler {
public:
	void on_message(connection_ptr con, message_ptr msg) {
		if (connections.find(con) != connections.end())
			connections[con]->send(msg->get_payload());
		else
			std::cerr << "that shouldn't have happened" << std::endl;
	}
	void on_open(connection_ptr con) {
		if (connections.find(con) == connections.end()) {
#ifdef DEBUG
			std::cout << "new connection, create new handler to process this message" << std::endl;
#endif
			connections[con] = new Connection(con, con->get_io_service());
		} else
			std::cerr << "did I just reuse a connection pointer???" << std::endl;
	}
	void on_close(connection_ptr con) {
		std::map<connection_ptr, Connection*>::iterator it = connections.find(con);
		if (it != connections.end()) {
			delete connections[con];
			connections.erase(it);
#ifdef DEBUG
			std::cout << "closing connection, delete corresponding handler" << connections.size() << std::endl;
#endif
		} else
			std::cerr << "that shouldn't have happened" << std::endl;
	}
private:
	std::map<connection_ptr, Connection*> connections;
};

int main(int argc, char* argv[]){
	boost::program_options::options_description description("Available options");
	description.add_options()
		("help", "produce this help message")
		("websocket-port", boost::program_options::value<unsigned short>(), "specify the port where the websocket server should listen.\nDefault is 1337")
		("MQTT-hostname", boost::program_options::value<std::string>(), "specify the host of the MQTT broker.\nDefault is localhost")
		("MQTT-port", boost::program_options::value<std::string>(), "specify the port where the MQTT broker listens.\nDefault is 1883");
	boost::program_options::variables_map variables_map;
	try {
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), variables_map);
		boost::program_options::notify(variables_map);
		if (variables_map.find("help") != variables_map.end()) {
			std::cout << description << "\n";
			return 1;
		}

		unsigned short websocketPort = variables_map.find("websocket-port") != variables_map.end() ? variables_map["websocket-port"].as<unsigned short>() : 1337;
		Connection::hostname = variables_map.find("MQTT-hostname") != variables_map.end() ? variables_map["MQTT-hostname"].as<std::string>() : "localhost";
		Connection::port = variables_map.find("MQTT-port") != variables_map.end() ? variables_map["MQTT-port"].as<std::string>() : "1883";

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
