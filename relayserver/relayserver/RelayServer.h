#pragma once

#include <uWS.h>
#include "TcpProtocol.h"
#include "WebsocketConnection.h"

class RelayServer
{
	public:
		RelayServer();
		int Run();
	private:
		int _clientSocket;
		TcpProtocol _tcpProtocol;
		std::string _statsHTTPResponse;

		static constexpr const char* ENV_GAMESERVER_HOST = "GAMESERVER_HOST";
		static constexpr const char* ENV_GAMESERVER_HOST_DEFAULT = "localhost";
		static constexpr const char* ENV_GAMESERVER_PORT = "GAMESERVER_PORT";
		static constexpr const char* ENV_GAMESERVER_PORT_DEFAULT = "9010";
		static constexpr const char* ENV_WEBSOCKET_PORT = "WEBSOCKET_PORT";
		static constexpr const char* ENV_WEBSOCKET_PORT_DEFAULT = "9009";
		static constexpr const size_t MAX_CLIENT_MESSAGE_SIZE = 10*1024;
		static int connectTcpSocket(const char* hostname, const char* port);
		static const char* getEnvOrDefault(const char* envVar, const char* defaultValue);
};
