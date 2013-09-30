WSS
===

TLS-enabled-MQTT-aware WebSocket to TCP Gateway

It creates a WebSocket server and forwards any WebSocket traffic to a TCP Server.
For every new WebSocket connection a TCP connection to the destination server is created and bound to the corresponding WebSocket connection.
This results in a 1:1 mapping of WebSocket and TCP connections and ensures that the responses of the TCP server arrive at the original WebSocket client.
TCP traffic from the broker is analyzed so that only complete MQTT messages will be send to the Websocket client; one MQTT message in every WebSocket message. 

It uses version 0.2 of websocket++ (https://github.com/zaphoyd/websocketpp/tree/0.2.x) and the Boost C++ Libraries (http://www.boost.org/).

This version depends on OpenSSL.
TLS support is still under development.
TLS1 should work on the websocket side.
MQTT broker side still unencrypted.
The ssl folder contains example key, cert, ca and dh files to play with.


The Websocket Server was developed using the eclipse IDE.
It assumes the boost and websocket++ header files and libraries are located in /usr/local/include and /usr/local/lib respectively. Boost headers are expected to be in /usr/include/boost and libs in /usr/lib/. If that doesn't match your setup just edit the eclipse project properties and/or makefile.

```$ make all``` will compile debug and release versions. For more details see makefile

Sample invocation

```$ ./WSS_release --brokerHost  192.168.11.31 --ws-keyfile ssl/server.key --ws-chainfile ssl/server.pem  --ws-dh-file ssl/dh.pem --broker-cert ssl/ca.pem``` this will use TLS on Websocket side and PLAIN TCP without encryprion on the MQTT broker side (to use TLS on broker side too --tls-version must be specified - but I haven't tested that yet and the given value is ignored anyway at the moment)

