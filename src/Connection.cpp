#include <websocketpp/sockets/tls.hpp>
#include <websocketpp/websocketpp.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "Socket.h"
#include "PlainSocket.h"
#include "TLSSocket.h"

#ifndef CONNECTION_H_
#define CONNECTION_H_

// The Connection created on construction a new TCP connection.
// It forwards incoming TCP traffic to the websocket. That happens MQTT aware
// Its send() method sends stuff (from the websocket) to the TCP endpoint

template <typename endpoint_type> // plain or tls
class Connection : public boost::enable_shared_from_this<Connection<endpoint_type> >{
public:

	typedef typename endpoint_type::handler::connection_ptr connection_ptr;

	Connection(connection_ptr con, boost::asio::io_service &io_service) :
		websocket_connection(con), readBuffer(1024), mqttMessage(""){
#ifdef DEBUG
		std::cout << "In Connection Constructor: creating new connection object" << std::endl;
#endif
		socket = NULL;
		if(Connection::tlsVersion == "" || Connection::brokerCert == ""){
			try {
				socket = Socket::create(io_service, NULL, Connection::hostname, Connection::port);
			} catch(boost::system::system_error &e){
				std::cerr << "foo " << e.what() << std::endl;
			}
		} else{
			try{
				boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv1_client);
				ctx.load_verify_file(Connection::brokerCert);
				socket = Socket::create(io_service, &ctx, Connection::hostname, Connection::port);
			} catch(boost::system::system_error &e){
				std::cerr << "bar " << e.what() << std::endl;
			}
		}
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
						socket->getSocketForAsio(),
						boost::asio::buffer(&next_byte, 1),
						boost::asio::transfer_at_least(1),
						boost::bind(
								&Connection::receive_remaining_length,
								this->shared_from_this(),
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
						socket->getSocketForAsio(),
						boost::asio::buffer(readBuffer, remaining_length),
						boost::asio::transfer_at_least(remaining_length),
						boost::bind(
								&Connection::receive_mqtt_message,
								this->shared_from_this(),
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
							socket->getSocketForAsio(),
							boost::asio::buffer(&next_byte, 1),
							boost::asio::transfer_at_least(1),
							boost::bind(
									&Connection::receive_remaining_length,
									this->shared_from_this(),
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
							socket->getSocketForAsio(),
							boost::asio::buffer(readBuffer, remaining_length),
							boost::asio::transfer_at_least(remaining_length),
							boost::bind(
									&Connection::receive_mqtt_message,
									this->shared_from_this(),
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
			socket->getSocketForAsio().write_some(boost::asio::buffer(message.c_str(), message.size()));
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
		if(socket){
			boost::asio::async_read(
				socket->getSocketForAsio(),
				boost::asio::buffer(mqtt_header, 2),
				boost::asio::transfer_at_least(2),
				boost::bind(
						&Connection::receive_header,
						this->shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
				)
		);
#ifdef DEBUG
		std::cout << "  started async TCP read" << std::endl;
#endif
		} else {
#ifdef DEBUG
		std::cout << "  failed to start async read because there is no tcp socket to broker" << std::endl;
#endif
		if(websocket_connection)
			websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem starting");
		}

	}

	void stop(){
#ifdef DEBUG
		std::cerr << "Beginning of stop() in Connection at " << this << std::endl;
#endif
		if(socket){
			socket->getSocketForAsio().cancel();
			socket->getSocketForAsio().close();
			delete socket;
		}
#ifdef DEBUG
		std::cout << "   stopped async TCP receive" << std::endl;
#endif
	}

	static std::string hostname;
	static std::string port;
	static std::string brokerCert;
	static std::string tlsVersion;

private:
	connection_ptr websocket_connection;
	Socket* socket;
	std::vector<char> readBuffer;
	char mqtt_header[2];
	char next_byte;
	unsigned int remaining_length;
	std::string mqttMessage;
	unsigned int multiplier;
};

template <typename endpoint_type>
std::string Connection<endpoint_type>::hostname;
template <typename endpoint_type>
std::string Connection<endpoint_type>::port;
template <typename endpoint_type>
std::string Connection<endpoint_type>::brokerCert;
template <typename endpoint_type>
std::string Connection<endpoint_type>::tlsVersion;

#endif /* CONNECTION_H_*/
