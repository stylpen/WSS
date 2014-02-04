all: WSS_debug WSS_release

WSS_debug:  build_debug/SharedBuffer.o build_debug/Socket.o build_debug/PlainSocket.o build_debug/TLSSocket.o build_debug/Connection.o build_debug/ServerHandler.o build_debug/WSS.o
	g++ -L/usr/local/lib -o WSS_debug  build_debug/SharedBuffer.o build_debug/Connection.o build_debug/PlainSocket.o build_debug/ServerHandler.o build_debug/Socket.o build_debug/TLSSocket.o build_debug/WSS.o  /usr/local/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/local/lib/libboost_thread.a /usr/local/lib/libboost_date_time.a /usr/local/lib/libboost_regex.a /usr/local/lib/libssl.a  /usr/local/lib/libcrypto.a -lrt -ldl -lpthread -static
	
WSS_release:  build_release/SharedBuffer.o build_release/Socket.o build_release/PlainSocket.o build_release/TLSSocket.o build_release/Connection.o build_release/ServerHandler.o build_release/WSS.o
	g++ -L/usr/local/lib -oWSS_release build_release/SharedBuffer.o build_release/Connection.o build_release/PlainSocket.o build_release/ServerHandler.o build_release/Socket.o build_release/TLSSocket.o build_release/WSS.o  /usr/local/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/local/lib/libboost_thread.a /usr/local/lib/libboost_date_time.a /usr/local/lib/libboost_regex.a /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a -lrt -ldl -lpthread -static 

build_debug/SharedBuffer.o: src/SharedBuffer.h src/SharedBuffer.cpp
	g++ -c -obuild_debug/SharedBuffer.o -Isrc -I/usr/local/include src/SharedBuffer.cpp -Wall -O3
	
build_debug/Socket.o: src/Socket.h src/Socket.cpp
	g++ -c -obuild_debug/Socket.o -Isrc -I/usr/local/include src/Socket.cpp -ggdb -DDEBUG
	
build_debug/PlainSocket.o: src/PlainSocket.h src/PlainSocket.cpp
	g++ -c -obuild_debug/PlainSocket.o -Isrc -I/usr/local/include src/PlainSocket.cpp -ggdb -DDEBUG

build_debug/TLSSocket.o: src/TLSSocket.h src/TLSSocket.cpp
	g++ -c -obuild_debug/TLSSocket.o -Isrc -I/usr/local/include src/TLSSocket.cpp -ggdb -DDEBUG

build_debug/Connection.o: src/Connection.cpp
	g++ -c -obuild_debug/Connection.o -Isrc -I/usr/local/include src/Connection.cpp -ggdb -DDEBUG

build_debug/ServerHandler.o: src/ServerHandler.cpp
	g++ -c -obuild_debug/ServerHandler.o -Isrc -I/usr/local/include src/ServerHandler.cpp -ggdb -DDEBUG

build_debug/WSS.o: src/WSS.cpp
	g++ -c -obuild_debug/WSS.o -Isrc -I/usr/local/include src/WSS.cpp -ggdb -DDEBUG
	
build_release/SharedBuffer.o: src/SharedBuffer.h src/SharedBuffer.cpp
	g++ -c -obuild_release/SharedBuffer.o -Isrc -I/usr/local/include src/SharedBuffer.cpp  -Wall -O3

build_release/Socket.o: src/Socket.h src/Socket.cpp
	g++ -c -obuild_release/Socket.o -Isrc -I/usr/local/include src/Socket.cpp  -Wall -O3
	
build_release/PlainSocket.o: src/PlainSocket.h src/PlainSocket.cpp
	g++ -c -obuild_release/PlainSocket.o -Isrc -I/usr/local/include src/PlainSocket.cpp  -Wall -O3

build_release/TLSSocket.o: src/TLSSocket.h src/TLSSocket.cpp
	g++ -c -obuild_release/TLSSocket.o -Isrc -I/usr/local/include src/TLSSocket.cpp  -Wall -O3

build_release/Connection.o: src/Connection.cpp
	g++ -c -obuild_release/Connection.o -Isrc -I/usr/local/include src/Connection.cpp  -Wall -O3

build_release/ServerHandler.o: src/ServerHandler.cpp
	g++ -c -obuild_release/ServerHandler.o -Isrc -I/usr/local/include src/ServerHandler.cpp  -Wall -O3

build_release/WSS.o: src/WSS.cpp
	g++ -c -obuild_release/WSS.o -Isrc -I/usr/local/include src/WSS.cpp -Wall -O3
	
clean_all: clean_debug clean_release

clean_debug:
	rm WSS_debug
	rm build_debug/*

clean_release:
	rm WSS_release
	rm build_release/*
