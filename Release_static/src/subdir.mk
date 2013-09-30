################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Connection.cpp \
../src/PlainSocket.cpp \
../src/ServerHandler.cpp \
../src/Socket.cpp \
../src/TLSSocket.cpp \
../src/WSS.cpp 

CPP_DEPS += \
./src/Connection.d \
./src/PlainSocket.d \
./src/ServerHandler.d \
./src/Socket.d \
./src/TLSSocket.d \
./src/WSS.d 

OBJS += \
./src/Connection.o \
./src/PlainSocket.o \
./src/ServerHandler.o \
./src/Socket.o \
./src/TLSSocket.o \
./src/WSS.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/local/include -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


