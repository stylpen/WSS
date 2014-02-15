all: WSS_debug WSS

WSS_debug: src/WSS.cpp
	g++ -ggdb -DDEBUG -o WSS_debug -I/usr/local/include src/WSS.cpp -Wall -O3 /usr/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/lib/libboost_thread.a /usr/lib/libboost_date_time.a /usr/lib/libboost_regex.a  -ldl -lpthread -static 

WSS: src/WSS.cpp
	g++ -o WSS -I/usr/local/include src/WSS.cpp -Wall -O3 /usr/lib/libboost_program_options.a /usr/local/lib/libwebsocketpp.a /usr/lib/libboost_system.a /usr/lib/libboost_thread.a /usr/lib/libboost_date_time.a /usr/lib/libboost_regex.a  -ldl -lpthread -static 
	
clean_all: clean_debug clean_release

clean_debug:
	rm WSS_debug

clean_release:
	rm WSS_release
