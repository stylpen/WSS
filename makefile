all: WSS_debug WSS

WSS_debug: src/WSS.cpp
	g++ -ggdb -DDEBUG -o WSS_debug -I/usr/local/include src/WSS.cpp -Wall -O3 /usr/local/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/local/lib/libboost_thread.a /usr/local/lib/libboost_date_time.a /usr/local/lib/libboost_regex.a /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a -lrt -ldl -lpthread -static 

WSS: src/WSS.cpp
	g++ -o WSS -I/usr/local/include src/WSS.cpp -Wall -O3 /usr/local/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/local/lib/libboost_thread.a /usr/local/lib/libboost_date_time.a /usr/local/lib/libboost_regex.a /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a -lrt -ldl -lpthread -static 
	
clean_all: clean_debug clean_release

clean_debug:
	rm WSS_debug

clean_release:
	rm WSS_release
