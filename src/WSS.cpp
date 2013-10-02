/*
* Copyright (c) 2012, Stephan Wypler. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of the WSS Project nor the
* names of its contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
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
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include "Connection.cpp"
#include "ServerHandler.cpp"
#include "Options.h"

#define VERSION "TLS MQTT VERSION BUILD 0.6"

Options options;

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

		options.broker_ca = variables_map.find("broker-certfile") != variables_map.end() ? variables_map["broker-certfile"].as<std::string>() : "";
		options.broker_hostname =  variables_map.find("brokerHost") != variables_map.end() ? variables_map["brokerHost"].as<std::string>() : "localhost";
		options.broker_port = variables_map.find("brokerPort") != variables_map.end() ? variables_map["brokerPort"].as<std::string>() : "1883";
		options.broker_tls_version = variables_map.find("tls-version") != variables_map.end() ? variables_map["tls-version"].as<std::string>() : "";
		options.broker_tls = (variables_map.find("tls-version") != variables_map.end() && variables_map.find("broker-certfile") != variables_map.end())? true : false;
		options.ws_crt = variables_map.find("ws-chainfile") != variables_map.end() ? variables_map["ws-chainfile"].as<std::string>() : "";
		options.ws_dh = variables_map.find("ws-dh-file") != variables_map.end() ? variables_map["ws-dh-file"].as<std::string>() : "";
		options.ws_key = variables_map.find("ws-keyfile") != variables_map.end() ? variables_map["ws-keyfile"].as<std::string>() : "";
		options.ws_tls = (variables_map.find("ws-keyfile") != variables_map.end() && variables_map.find("ws-chainfile") != variables_map.end()) ? true : false;

		unsigned short websocketPort = variables_map.find("websocketPort") != variables_map.end() ? variables_map["websocketPort"].as<unsigned short>() : 18883;

		if(options.ws_tls){
			Connection<websocketpp::server_tls>::brokerCert = variables_map.find("broker-certfile") != variables_map.end() ? variables_map["broker-certfile"].as<std::string>() : "";
			Connection<websocketpp::server_tls>::tlsVersion = variables_map.find("tls-version") != variables_map.end() ? variables_map["tls-version"].as<std::string>() : "";

			websocketpp::server_tls::handler::ptr serverHandler(new TLSServerHandler());
			websocketpp::server_tls websocketServer(serverHandler);

			boost::asio::signal_set signals(websocketServer.get_io_service(), SIGINT, SIGTERM);
			signals.async_wait(boost::bind(&websocketpp::server_tls::stop, boost::ref(websocketServer), true, websocketpp::close::status::NORMAL, "websocket server quit"));

			websocketServer.alog().unset_level(websocketpp::log::alevel::ALL);
			websocketServer.elog().unset_level(websocketpp::log::elevel::ALL);
			if (variables_map.find("verbose") != variables_map.end()) {
				websocketServer.elog().set_level(websocketpp::log::elevel::RERROR);
				websocketServer.elog().set_level(websocketpp::log::elevel::FATAL);
			}
			std::cerr << "WS TLS" <<std::endl;

			websocketServer.listen(boost::asio::ip::tcp::v4(), websocketPort, 1);
		}else{

			Connection<websocketpp::server>::brokerCert = variables_map.find("broker-certfile") != variables_map.end() ? variables_map["broker-certfile"].as<std::string>() : "";
			Connection<websocketpp::server>::tlsVersion = variables_map.find("tls-version") != variables_map.end() ? variables_map["tls-version"].as<std::string>() : "";

			websocketpp::server::handler::ptr serverHandler(new PlainServerHandler());
			websocketpp::server websocketServer(serverHandler);

			boost::asio::signal_set signals(websocketServer.get_io_service(), SIGINT, SIGTERM);
			signals.async_wait(boost::bind(&websocketpp::server::stop, boost::ref(websocketServer), true, websocketpp::close::status::NORMAL, "websocket server quit"));

			std::cerr << "WS NON TLS" <<std::endl;

			websocketServer.alog().unset_level(websocketpp::log::alevel::ALL);
			websocketServer.elog().unset_level(websocketpp::log::elevel::ALL);
			if (variables_map.find("verbose") != variables_map.end()) {
				websocketServer.elog().set_level(websocketpp::log::elevel::RERROR);
				websocketServer.elog().set_level(websocketpp::log::elevel::FATAL);
			}
			websocketServer.listen(boost::asio::ip::tcp::v4(), websocketPort, 1);
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
