WSS
===

MQTT- aware WebSocket to TCP Gateway

It creates a WebSocket server and forwards any WebSocket traffic to a TCP Server.
For every new WebSocket connection a TCP connection to the destination server is created and bound to the corresponding WebSocket connection.
This results in a 1:1 mapping of WebSocket and TCP connections and ensures that the responses of the TCP server arrive at the original WebSocket client.
TCP traffic from the broker is analyzed so that only complete MQTT messages will be send to the Websocket client; one MQTT message in every WebSocket message. 

It uses version 0.2 of websocket++ (https://github.com/zaphoyd/websocketpp/tree/0.2.x) and the Boost C++ Libraries (http://www.boost.org/).

The Makefile assumes you have boost installed in /usr/local. If your setup is different set ```BOOST_PREFIX=<your boost location>``` accordingly. Please refer to the Makefile for more information.
By default a statically linked binary will be created. ```make SHARED=1``` will create a dynamically linked one. You can optionally pass ```DEBUG=1``` to ```make``` in order to get very verbose output (not recommended).

You can also build manually:
To create an executale that uses shared libraries run (for example)

g++ -o WSS_shared -I/usr/local/include -L/usr/local/lib -lpthread -lboost_program_options -lboost_regex -lboost_thread -lboost_system -lwebsocketpp  src/WSS.cpp -O3 -Wall

Make sure that you have all symlinks you need from libwebsocketpp.so.* to libwebsocketpp.so and to libwebsocketpp.so.0 and that the LD_LIBRARY_PATH contains the shared libraries when you execute the program.
To create a static linked executable run

g++ -o WSS_static src/WSS.cpp -I/usr/local/include  /usr/local/lib/libwebsocketpp.a /usr/local/lib/libboost_thread.a /usr/local/lib/libboost_regex.a /usr/local/lib/libboost_system.a /usr/local/lib/libboost_program_options.a -lpthread -O3 -Wall

