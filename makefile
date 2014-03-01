all: gitSubmodules openssl websocketpp WSS

gitSubmodules:
	git submodule init 
	git submodule update
	
openssl: gitSubmodules
	cd openssl; \
	./config && \
	make
	
websocketpp: gitSubmodules
	cd websocketpp; \
	make
	

BOOST_PREFIX ?= /usr/local
BOOST_LIB_PATH		?= $(BOOST_PREFIX)/lib
BOOST_INCLUDE_PATH  ?= $(BOOST_PREFIX)/include


WSS: build_release/SharedBuffer.o build_release/Socket.o build_release/PlainSocket.o build_release/TLSSocket.o build_release/Connection.o build_release/ServerHandler.o build_release/WSS.o 
	g++ -oWSS build_release/SharedBuffer.o build_release/Connection.o build_release/PlainSocket.o build_release/ServerHandler.o build_release/Socket.o build_release/TLSSocket.o build_release/WSS.o websocketpp/libwebsocketpp.a $(BOOST_LIB_PATH)/libboost_program_options.a $(BOOST_LIB_PATH)/libboost_system.a $(BOOST_LIB_PATH)/libboost_thread.a $(BOOST_LIB_PATH)/libboost_date_time.a $(BOOST_LIB_PATH)/libboost_regex.a openssl/libssl.a openssl/libcrypto.a -ldl -lrt -lpthread -static 
	
build_release/SharedBuffer.o: src/SharedBuffer.h src/SharedBuffer.cpp
	g++ -c -obuild_release/SharedBuffer.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/SharedBuffer.cpp  -Wall -O3

build_release/Socket.o: src/Socket.h src/Socket.cpp
	g++ -c -obuild_release/Socket.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/Socket.cpp  -Wall -O3
	
build_release/PlainSocket.o: src/PlainSocket.h src/PlainSocket.cpp
	g++ -c -obuild_release/PlainSocket.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/PlainSocket.cpp  -Wall -O3

build_release/TLSSocket.o: src/TLSSocket.h src/TLSSocket.cpp
	g++ -c -obuild_release/TLSSocket.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/TLSSocket.cpp  -Wall -O3

build_release/Connection.o: src/Connection.cpp
	g++ -c -obuild_release/Connection.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/Connection.cpp  -Wall -O3

build_release/ServerHandler.o: src/ServerHandler.cpp
	g++ -c -obuild_release/ServerHandler.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/ServerHandler.cpp  -Wall -O3

build_release/WSS.o: src/WSS.cpp
	g++ -c -obuild_release/WSS.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/WSS.cpp -Wall -O3
	
	
WSS_debug:  build_debug/SharedBuffer.o build_debug/Socket.o build_debug/PlainSocket.o build_debug/TLSSocket.o build_debug/Connection.o build_debug/ServerHandler.o build_debug/WSS.o
	g++ -L/usr/local/lib -o WSS_debug  build_debug/SharedBuffer.o build_debug/Connection.o build_debug/PlainSocket.o build_debug/ServerHandler.o build_debug/Socket.o build_debug/TLSSocket.o build_debug/WSS.o websocketpp/libwebsocketpp.a $(BOOST_LIB_PATH)/libboost_program_options.a $(BOOST_LIB_PATH)/libboost_system.a $(BOOST_LIB_PATH)/libboost_thread.a $(BOOST_LIB_PATH)/libboost_date_time.a $(BOOST_LIB_PATH)/libboost_regex.a openssl/libssl.a openssl/libcrypto.a -ldl -lrt -lpthread -static 

build_debug/SharedBuffer.o: src/SharedBuffer.h src/SharedBuffer.cpp
	g++ -c -obuild_debug/SharedBuffer.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/SharedBuffer.cpp -Wall -DDEBUG
	
build_debug/Socket.o: src/Socket.h src/Socket.cpp
	g++ -c -obuild_debug/Socket.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/Socket.cpp -ggdb -DDEBUG
	
build_debug/PlainSocket.o: src/PlainSocket.h src/PlainSocket.cpp
	g++ -c -obuild_debug/PlainSocket.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/PlainSocket.cpp -ggdb -DDEBUG

build_debug/TLSSocket.o: src/TLSSocket.h src/TLSSocket.cpp
	g++ -c -obuild_debug/TLSSocket.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/TLSSocket.cpp -ggdb -DDEBUG

build_debug/Connection.o: src/Connection.cpp
	g++ -c -obuild_debug/Connection.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/Connection.cpp -ggdb -DDEBUG

build_debug/ServerHandler.o: src/ServerHandler.cpp
	g++ -c -obuild_debug/ServerHandler.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/ServerHandler.cpp -ggdb -DDEBUG

build_debug/WSS.o: src/WSS.cpp
	g++ -c -obuild_debug/WSS.o -Isrc -I$(BOOST_INCLUDE_PATH) -Iwebsocketpp/src -Iopenssl/include src/WSS.cpp -ggdb -DDEBUG
	
clean_all: clean_debug clean_release clean_openssl clean_websocketpp

clean_debug:
	rm WSS_debug
	rm build_debug/*

clean_release:
	rm WSS_release
	rm build_release/*

clean_openssl:
	cd openssl; \
	make clean

clean_websocketpp:
	cd websocketpp; \
	make clean
