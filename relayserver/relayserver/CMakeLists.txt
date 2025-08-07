set(PROJECT_NAME RelayServer)
project (${PROJECT_NAME} VERSION 0.1 LANGUAGES CXX)

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

include_directories(
	${PROJECT_NAME}
	../lib/uWebSockets/src
	../lib/msgpack-c/include
	../lib/nlohmann/single_include
)

add_executable(
	${PROJECT_NAME}
	main.cpp
	RelayServer.h RelayServer.cpp
	TcpProtocol.h TcpProtocol.cpp
	MsgPackProtocol.h MsgPackProtocol.cpp
	JsonProtocol.h JsonProtocol.cpp
	WebsocketConnection.h WebsocketConnection.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	tcpserver
	uWebSockets
	pthread
	ssl
	crypto
	z
)
