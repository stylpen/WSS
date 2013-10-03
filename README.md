WSS
===

TLS-enabled-MQTT-aware WebSocket to TCP Gateway

It creates a WebSocket server and forwards any WebSocket traffic to a TCP Server.
For every new WebSocket connection a TCP connection to the destination server is created and bound to the corresponding WebSocket connection.
This results in a 1:1 mapping of WebSocket and TCP connections and ensures that the responses of the TCP server arrive at the original WebSocket client.
TCP traffic from the broker is analyzed so that only complete MQTT messages will be send to the Websocket client; one MQTT message in every WebSocket message. 

It uses version 0.2 of websocket++ (https://github.com/zaphoyd/websocketpp/tree/0.2.x) and the Boost C++ Libraries (http://www.boost.org/).

This version depends on OpenSSL.
The ssl folder contains example key, cert, ca and dh files to play with.


The Websocket Server was developed using the eclipse IDE.
It assumes the boost and websocket++ header files and libraries are located in /usr/local/include and /usr/local/lib respectively. Boost headers are expected to be in /usr/include/boost and libs in /usr/lib/. If that doesn't match your setup just edit the eclipse project properties and/or makefile.

```$ make all``` will compile debug and release versions. For more details see makefile

By default TLSv1 is used if all required certificates/keys are provided.

These are for the Websocket side: --ws-keyfile <websocket server key file> --ws-chainfile <websocket server certificate file> and optionally --ws-dh-file <diffie-hellman parameter file>

For the MQTT broker side provide --broker-cert <MQTT broker CA>. WARNING: THIS IS CURRENTLY NOT USED TO CHECK AUTHENTICITY OF THE REMOTE PEER. Any file will have the same effect: TLS is used for the connection but NO remote peer is verified using the CA.

To use TLS versions other than TLSv1 specify --ws-tls-version <version> and/or --broker-tls-version <version>. This only works if you compiled with boost version 1.54 or later. Otherwise TLSv1 is used regardless of the specified version.
Use 'TLSv11' for version 1.1 or 'TLSv12' for version 1.2

sample invocation:

```$ ./WSS_release --brokerHost localhost --ws-keyfile ssl/server.key --ws-chainfile ssl/server.pem  --ws-dh-file ssl/dh.pem --broker-cert ssl/ca.pem```
