WSS
===

MQTT- aware WebSocket to TCP Gateway

It creates a WebSocket server and forwards any WebSocket traffic to a TCP Server.
For every new WebSocket connection a TCP connection to the destination server is created and bound to the corresponding WebSocket connection.
This results in a 1:1 mapping of WebSocket and TCP connections and ensures that the responses of the TCP server arrive at the original WebSocket client.
TCP traffic from the broker is analyzed so that only complete MQTT messages will be send to the Websocket client; one MQTT message in every WebSocket message. 

It uses version 0.2 of websocket++ (https://github.com/zaphoyd/websocketpp/tree/0.2.x) and the Boost C++ Libraries (http://www.boost.org/).

The Websocket Server was developed using the eclipse IDE.
It assumes the boost and websocket++ header files and libraries are located in /usr/local/include and /usr/local/lib respectively. Boost headers are expected to be in /usr/include/ and libs in /usr/lib/. If that doesn't match your setup just edit the eclipse project properties.

You can also build manually:
To create an executale that uses shared libraries run

g++ -o WSS_shared -I/usr/local/include -L/usr/local/lib -lpthread -lboost_program_options -lboost_regex -lboost_thread -lboost_system -lwebsocketpp  src/WSS.cpp -O3 -Wall

Make sure that you have all symlinks you need from libwebsocketpp.so.* to libwebsocketpp.so and to libwebsocketpp.so.0 and that the LD_LIBRARY_PATH contains the shared libraries when you execute the program.
To create a static linked executable run

g++ -o WSS_static src/WSS.cpp -I/usr/local/include  /usr/local/lib/libwebsocketpp.a /usr/local/lib/libboost_thread.a /usr/local/lib/libboost_regex.a /usr/local/lib/libboost_system.a /usr/local/lib/libboost_program_options.a -lpthread -O3 -Wall

