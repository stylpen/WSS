all: WSS_debug WSS_release

WSS_debug:  build_debug/Socket.o build_debug/PlainSocket.o build_debug/TLSSocket.o build_debug/Connection.o build_debug/ServerHandler.o build_debug/WSS.o
	g++ -L/usr/local/lib -o WSS_debug  build_debug/Connection.o build_debug/PlainSocket.o build_debug/ServerHandler.o build_debug/Socket.o build_debug/TLSSocket.o build_debug/WSS.o  /usr/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/lib/libboost_thread.a /usr/lib/libboost_date_time.a /usr/lib/libboost_regex.a -lpthread -lcrypto -lssl
	
WSS_release:  build_release/Socket.o build_release/PlainSocket.o build_release/TLSSocket.o build_release/Connection.o build_release/ServerHandler.o build_release/WSS.o
	g++ -L/usr/local/lib -oWSS_release  build_release/Connection.o build_release/PlainSocket.o build_release/ServerHandler.o build_release/Socket.o build_release/TLSSocket.o build_release/WSS.o  /usr/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/lib/libboost_thread.a /usr/lib/libboost_date_time.a /usr/lib/libboost_regex.a -lpthread -lcrypto -lssl
	
build_debug/Socket.o: src/Socket.h src/Socket.cpp
	g++ -c -obuild_debug/Socket.o -Isrc src/Socket.cpp -ggdb -DDEBUG
	
build_debug/PlainSocket.o: src/PlainSocket.h src/PlainSocket.cpp
	g++ -c -obuild_debug/PlainSocket.o -Isrc src/PlainSocket.cpp -ggdb -DDEBUG

build_debug/TLSSocket.o: src/TLSSocket.h src/TLSSocket.cpp
	g++ -c -obuild_debug/TLSSocket.o -Isrc src/TLSSocket.cpp -ggdb -DDEBUG

build_debug/Connection.o: src/Connection.cpp
	g++ -c -obuild_debug/Connection.o -Isrc src/Connection.cpp -ggdb -DDEBUG

build_debug/ServerHandler.o: src/ServerHandler.cpp
	g++ -c -obuild_debug/ServerHandler.o -Isrc src/ServerHandler.cpp -ggdb -DDEBUG

build_debug/WSS.o: src/WSS.cpp
	g++ -c -obuild_debug/WSS.o -Isrc src/WSS.cpp -ggdb -DDEBUG
	
build_release/Socket.o: src/Socket.h src/Socket.cpp
	g++ -c -obuild_release/Socket.o -Isrc src/Socket.cpp  -Wall -O3
	
build_release/PlainSocket.o: src/PlainSocket.h src/PlainSocket.cpp
	g++ -c -obuild_release/PlainSocket.o -Isrc src/PlainSocket.cpp  -Wall -O3

build_release/TLSSocket.o: src/TLSSocket.h src/TLSSocket.cpp
	g++ -c -obuild_release/TLSSocket.o -Isrc src/TLSSocket.cpp  -Wall -O3

build_release/Connection.o: src/Connection.cpp
	g++ -c -obuild_release/Connection.o -Isrc src/Connection.cpp  -Wall -O3

build_release/ServerHandler.o: src/ServerHandler.cpp
	g++ -c -obuild_release/ServerHandler.o -Isrc src/ServerHandler.cpp  -Wall -O3

build_release/WSS.o: src/WSS.cpp
	g++ -c -obuild_release/WSS.o -Isrc src/WSS.cpp -Wall -O3
	
clean_all: clean_debug clean_release

clean_debug:
	rm WSS_debug
	rm build_debug/*

clean_release:
	rm WSS_release
	rm build_release/*
