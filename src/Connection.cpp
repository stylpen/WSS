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
#include "SharedBuffer.h"
#include "Options.h"
extern Options options;

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
		websocket_connection(con), readBuffer(1024), mqttMessage(""), connected(false), closed(false){
#ifdef DEBUG
		std::cerr << "In Connection Constructor: creating socket" << std::endl;
#endif
		socket = Socket::create(io_service);
	}

	~Connection(){
#ifdef DEBUG
		std::cerr << "Beginning of Destructor of Connection" << std::endl << "   Connection address is: " << this << " websocket: " << websocket_connection << std::endl;
#endif
		websocket_connection->close(websocketpp::close::status::NORMAL, "closing");
#ifdef DEBUG
		std::cerr << "End of Destructor of Connection. Address was " << this << " (" << websocket_connection << ")" << std::endl;
#endif
	}

	void init(){
#ifdef DEBUG
		std::cerr << "Init Connection: connecting to broker" << std::endl;
#endif
		socket->on_fail = boost::bind(&Connection::fail, this->shared_from_this());
		socket->on_success = boost::bind(&Connection::first_start, this->shared_from_this());
		socket->on_end = boost::bind(&Connection::cleanup, this->shared_from_this());
		socket->do_connect();
	}

	void first_start(){
#ifdef DEBUG
		std::cerr << "broker connection established" << std::endl;
		std::cerr << "queued stuff: " << queued_messages.size() << " bytes: " ;
		unsigned int i = 0;
		for(std::string::const_iterator it = queued_messages.begin(); it != queued_messages.end() && i <= queued_messages.size(); it++, i++)
			std::cerr << std::setw(2) << std::setfill('0') << std::hex << (short)*it << " ";
		std::cerr << std::dec << std::endl;
#endif
		socket->async_write(
			SharedBuffer(queued_messages),
			boost::bind(
				&Connection::async_tcp_write_handler,
				this->shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		);
#ifdef DEBUG
		std::cerr << "sending queued messages" << queued_messages << std::endl;
#endif
		connected = true;
		start_receive();
	}

	void fail(){
#ifdef DEBUG
		std::cerr << "socket error in connection " << this << std::endl;
#endif
		try{
			if(!closed)
				websocket_connection->close(websocketpp::close::status::NORMAL, "failed");
		}catch(std::exception &e){
			std::cerr << e.what() << std::endl;
		}
	}

	void cleanup(){
#ifdef DEBUG
		std::cerr << "in cleanup of connection" << std::endl;
#endif
		if(socket->getSocketForAsio().is_open()){
			socket->getSocketForAsio().cancel();
			socket->getSocketForAsio().close();
		}
		socket.reset();
#ifdef DEBUG
		std::cerr << "   stopped async TCP receive" << std::endl;
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
			std::cerr << "received header from broker." << std::endl
					<< "Length: " << bytes_transferred << " Bytes" << std::endl
					<< std::setw(2) << std::setfill('0') << std::hex
					<< "Payload: " << (short)mqtt_header[0] << " " << (short)mqtt_header[1] << std::endl << std::endl << std::dec;
#endif
			mqttMessage = std::string(mqtt_header, bytes_transferred);
			if(mqtt_header[1] == 0){ // nothing following remaining length is 0 maybe that was a ping or something.
#ifdef DEBUG
			std::cerr << "there is nothing more -> sending it to the websocket." << std::endl << std::endl;
#endif
			websocket_connection->send(mqttMessage,	websocketpp::frame::opcode::BINARY);
			// wait for new message
			start_receive();
			} else if(mqtt_header[1] & 0x80){ // the length was not 0 and we need to read more remaining length bytes. receive_remaining_length will do it ...
#ifdef DEBUG
				std::cerr << "need to read extra bytes for remaining length field" << std::endl << std::endl;
#endif
				multiplier = 1;
				remaining_length = mqtt_header[1] & 0x7F;
#ifdef DEBUG
				std::cerr << "length initially is: " << remaining_length << std::endl;
#endif
				socket->async_read(
					boost::asio::buffer(&next_byte, 1),
					1,
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
				std::cerr << "the remaining message length is: " << remaining_length << std::endl << std::endl;
#endif
				readBuffer.resize(remaining_length);
				socket->async_read(
					boost::asio::buffer(readBuffer, remaining_length),
					remaining_length,
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
			std::cerr << "stopped receiving header " << std::endl << bytes_transferred << " bytes arrived so far. error " << error << " websocket: " << websocket_connection << " connection " << this << std::endl << std::endl;
#endif
			if(error && error != boost::asio::error::operation_aborted){
				if(options.verbose)
					std::cerr << "error mqtt header " << error.message() << std::endl;
				websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem while reading MQTT header");
			}
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
			std::cerr << "received one more byte with remaining length: "
					<< std::setw(2) << std::setfill('0') << std::hex
					<< (short)next_byte << std::endl << std::dec;
#endif
				mqttMessage.append(std::string(&next_byte, 1));
#ifdef DEBUG
				std::cerr << "there is still work to do" << mqtt_header[1] << std::endl << std::endl;
#endif
				multiplier *= 128;
				remaining_length += (next_byte & 0x7F) * multiplier;
#ifdef DEBUG
				std::cerr << "remaining length is now: " << remaining_length << std::endl;
#endif
				if(next_byte & 0x80){ // there is still a continuation bit and we need to go on by calling this method again.
					socket->async_read(
						boost::asio::buffer(&next_byte, 1),
						1,
						boost::bind(
							&Connection::receive_remaining_length,
							this->shared_from_this(),
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred
						)
					);
				} else { // this was the last one and we now know the length of the remaining message. receive_mqtt_message will now do the rest
#ifdef DEBUG
					std::cerr << "length finally is: "  << remaining_length << std::endl;
#endif
					readBuffer.resize(remaining_length);
					socket->async_read(
					boost::asio::buffer(readBuffer, remaining_length),
						remaining_length,
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
				std::cerr << "error reading length bytes " << std::endl << bytes_transferred << "bytes arrived so far " << error << " websocket: " << websocket_connection << " connection " << this << std::endl << std::endl;
#endif
				if(error && error != boost::asio::error::operation_aborted){
					if(options.verbose)
						std::cerr << "error reading remaining length of mqtt message " << error.message() << std::endl;
					websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem while reading remaining length");
				}
			}
		}


	/*
	 * Gets called when remaining_length bytes were read from the socket.
	 * Appends the stuff to the mqtt_message string and sends it via websocket
	 */
	void receive_mqtt_message(const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error && websocket_connection->get_state() == websocketpp::session::state::OPEN){
#ifdef DEBUG
			std::cerr << "received message body from broker " << bytes_transferred << " bytes: ";
			for(std::vector<char>::iterator it = readBuffer.begin(); it != readBuffer.end(); it++)
				std::cerr << std::setw(2) << std::setfill('0') << std::hex << (short)*it << " ";
			std::cerr << std::dec << std::endl;
		    std::cerr << "plaintext: " << std::string(readBuffer.data(), bytes_transferred) << std::endl;
#endif
			mqttMessage.append(std::string(readBuffer.data(), bytes_transferred));
			websocket_connection->send(mqttMessage, websocketpp::frame::opcode::BINARY);
			// wait for new message
			start_receive();
		} else {
#ifdef DEBUG
			std::cerr << "error reading mqtt message (or operation was canceled)" << std::endl << bytes_transferred << "bytes arrived so far " << error << " websocket: " << websocket_connection << " connection " << this << std::endl << std::endl;
#endif
			if(error && error != boost::asio::error::operation_aborted){
				if(options.verbose)
					std::cerr << "error reading mqtt message " << error.message() << std::endl;
				websocket_connection->close(websocketpp::close::status::NORMAL, "connection problem in receice_message");
			}
		}
	}

	void async_tcp_write_handler(const boost::system::error_code& error, std::size_t bytes_transferred){
		if(error){
			if(options.verbose)
				std::cerr << "error writing to the broker " << error.message() << std::endl;
			if(websocket_connection)
				websocket_connection->close(websocketpp::close::status::NORMAL, "problem while writing to the broker");
		}
	}

	void send(const std::string &message) {
#ifdef DEBUG
		std::cerr << "sent TCP segment to broker " << message.size() << " bytes: " ;
		unsigned int i = 0;
		for(std::string::const_iterator it = message.begin(); it != message.end() && i <= message.size(); it++, i++)
			std::cerr << std::setw(2) << std::setfill('0') << std::hex << (short)*it << " ";
		std::cerr << std::dec << std::endl;
		std::cerr << "plaintext: " << message << std::endl;
#endif
		if(connected){
#ifdef DEBUG
			std::cerr << "broker connection had been established. sending this message ..." << std::endl;
#endif
			socket->async_write(
				SharedBuffer(message),
				boost::bind(
					&Connection::async_tcp_write_handler,
					this->shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred
				)
			);
		} else {
#ifdef DEBUG
			std::cerr << "broker connection has not been established yet. queuing this message ..." << std::endl;
#endif
			queued_messages.append(message);
		}
	}

	void start_receive() {
#ifdef DEBUG
		std::cerr << "Begin of start_receive() in Connection at: " << this << std::endl;
#endif
		socket->async_read(
			boost::asio::buffer(mqtt_header, 2),
			2,
			boost::bind(
				&Connection::receive_header,
				this->shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		);
#ifdef DEBUG
		std::cerr << "  started async TCP read" << std::endl;
#endif
	}

	void stop(){
#ifdef DEBUG
		std::cerr << "stop() in Connection at " << this << std::endl;
#endif
		socket->end();
		closed = true;
	}

private:
	connection_ptr websocket_connection;
	std::string queued_messages;
	boost::shared_ptr<Socket> socket;
	std::vector<char> readBuffer;
	char mqtt_header[2];
	char next_byte;
	unsigned int remaining_length;
	std::string mqttMessage;
	unsigned int multiplier;
	bool connected;
	bool closed;
};

#endif /* CONNECTION_H_*/
