# general setup
COMMON_CXX_FLAGS = -Wall -fstack-protector

ifeq ($(DEBUG), 1)
	BIN_NAME = WSS_debug
	WORK_DIR = build_debug
	CXX_FLAGS = $(COMMON_CXX_FLAGS) -ggdb -DDEBUG
else
	BIN_NAME = WSS
	WORK_DIR = build_release
	CXX_FLAGS = $(COMMON_CXX_FLAGS) -Wall -O3
endif

# boost
BOOST_PREFIX ?= /usr
BOOST_LIB_PATH		?= $(BOOST_PREFIX)/lib
BOOST_INCLUDE_PATH  ?= $(BOOST_PREFIX)/include

BOOST_LIBS = boost_program_options boost_system boost_thread boost_date_time boost_regex

ifeq ($(wildcard $(BOOST_LIB_PATH)/libboost*-mt.a),)
	BOOST_STATIC_LIBS := $(foreach LIB, $(BOOST_LIBS), $(BOOST_LIB_PATH)/lib$(LIB).a)
	BOOST_DYNAMIC_LIBS := -L$(BOOST_LIB_PATH) $(foreach LIB, $(BOOST_LIBS), -l$(LIB))
else
	BOOST_STATIC_LIBS := $(foreach LIB, $(BOOST_LIBS), $(BOOST_LIB_PATH)/lib$(LIB)-mt.a)
	BOOST_DYNAMIC_LIBS := -L$(BOOST_LIB_PATH) $(foreach LIB, $(BOOST_LIBS), -l$(LIB)-mt)
endif


# openssl
OPENSSL_LIB_PATH		?= openssl
OPENSSL_INCLUDE_PATH	?= openssl/include

# shared or static
ifeq ($(SHARED), 1)
	BIN_NAME := $(BIN_NAME)_shared
	LINKER_SETTINGS = -Lwebsocketpp -L$(OPENSSL_LIB_PATH) -lwebsocketpp $(BOOST_DYNAMIC_LIBS) -lcrypto -lssl -ldl -lpthread
else
	LINKER_SETTINGS = websocketpp/libwebsocketpp.a $(BOOST_STATIC_LIBS) $(OPENSSL_LIB_PATH)/libssl.a $(OPENSSL_LIB_PATH)/libcrypto.a -ldl -lpthread -static
endif


INCLUDES = -I$(BOOST_INCLUDE_PATH) -Isrc -Iwebsocketpp/src -I$(OPENSSL_INCLUDE_PATH)


all: openssl websocketpp $(BIN_NAME)

gitSubmodules:
	git submodule init 
	git submodule update
	
openssl: gitSubmodules
	cd openssl; \
	./config shared && \
	make depend &&\
	make
	
websocketpp: gitSubmodules
	cd websocketpp; \
	make; \
	make SHARED=1

$(BIN_NAME): $(WORK_DIR)/SharedBuffer.o $(WORK_DIR)/Socket.o $(WORK_DIR)/PlainSocket.o $(WORK_DIR)/TLSSocket.o $(WORK_DIR)/Connection.o $(WORK_DIR)/ServerHandler.o $(WORK_DIR)/WSS.o
	$(CXX) -o $(BIN_NAME) $(WORK_DIR)/SharedBuffer.o $(WORK_DIR)/Connection.o $(WORK_DIR)/PlainSocket.o $(WORK_DIR)/ServerHandler.o $(WORK_DIR)/Socket.o $(WORK_DIR)/TLSSocket.o $(WORK_DIR)/WSS.o $(LINKER_SETTINGS)
	
$(WORK_DIR)/SharedBuffer.o: src/SharedBuffer.h src/SharedBuffer.cpp
	$(CXX) -c -o $(WORK_DIR)/SharedBuffer.o $(INCLUDES) src/SharedBuffer.cpp $(CXX_FLAGS)

$(WORK_DIR)/Socket.o: src/Socket.h src/Socket.cpp
	$(CXX) -c -o $(WORK_DIR)/Socket.o $(INCLUDES) src/Socket.cpp $(CXX_FLAGS)
	
$(WORK_DIR)/PlainSocket.o: src/PlainSocket.h src/PlainSocket.cpp
	$(CXX) -c -o $(WORK_DIR)/PlainSocket.o $(INCLUDES) src/PlainSocket.cpp $(CXX_FLAGS)

$(WORK_DIR)/TLSSocket.o: src/TLSSocket.h src/TLSSocket.cpp
	$(CXX) -c -o $(WORK_DIR)/TLSSocket.o $(INCLUDES) src/TLSSocket.cpp $(CXX_FLAGS)

$(WORK_DIR)/Connection.o: src/Connection.cpp
	$(CXX) -c -o $(WORK_DIR)/Connection.o $(INCLUDES) src/Connection.cpp $(CXX_FLAGS)

$(WORK_DIR)/ServerHandler.o: src/ServerHandler.cpp
	$(CXX) -c -o $(WORK_DIR)/ServerHandler.o $(INCLUDES) src/ServerHandler.cpp $(CXX_FLAGS)

$(WORK_DIR)/WSS.o: src/WSS.cpp
	$(CXX) -c -o $(WORK_DIR)/WSS.o $(INCLUDES) src/WSS.cpp $(CXX_FLAGS)
	
clean: clean_debug clean_release clean_openssl clean_websocketpp

clean_debug:
	rm -f WSS_debug WSS_debug_static
	rm -f build_debug/*

clean_release:
	rm -f WSS WSS_static
	rm -f build_release/*

clean_openssl:
	cd openssl; \
	make clean

clean_websocketpp:
	cd websocketpp; \
	make clean
