#include <boost/shared_ptr.hpp>
#include <boost/asio/buffer.hpp>
#include <string>
#include <vector>
#include <iostream>

#ifndef SHAREDBUFFER_H_
#define SHAREDBUFFER_H_


// from http://www.boost.org/doc/libs/1_54_0/doc/html/boost_asio/example/cpp03/buffers/reference_counted.cpp
class SharedBuffer {

public:
  explicit SharedBuffer(const std::string&);

  // Implement the ConstBufferSequence requirements.
  typedef boost::asio::const_buffer value_type;
  typedef const boost::asio::const_buffer* const_iterator;
  const boost::asio::const_buffer* begin() const;
  const boost::asio::const_buffer* end() const;

private:
  boost::shared_ptr<std::vector<char> > data;
  boost::asio::const_buffer buffer;

};

#endif /* SHAREDBUFFER_H_ */
