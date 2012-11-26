WSS
===

WebSocket to TCP Gateway

WSS is a WebSocket Server to TCP Gateway that was originally created to connect mosquitto.js to an MQTT broker.
It can be used to forward any WebSocket traffic to a TCP Server.
For every new WebSocket connection a TCP connection to the destination server is created and bound to the corresponding WebSocket connection.
This results in a 1:1 mapping of WebSocket and TCP connections and ensures that the responses of the TCP server arrive at the original WebSocket Client

It uses websocketpp (https://github.com/zaphoyd/websocketpp) and the Boost C++ Libraries (http://www.boost.org/).
