#include "SharedBuffer.h"

SharedBuffer::SharedBuffer(const std::string& message)
	: data(new std::vector<char>(message.begin(), message.end())),
	  buffer(boost::asio::buffer(*data)){
 }

const boost::asio::const_buffer* SharedBuffer::begin() const{
	return &buffer;
}

const boost::asio::const_buffer* SharedBuffer::end() const{
	return &buffer + 1;
}
