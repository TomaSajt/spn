#include "WebsocketConnection.h"
#include "TcpProtocol.h"
#include "MsgPackProtocol.h"
#include "JsonProtocol.h"

WebsocketConnection::WebsocketConnection(uWS::WebSocket<uWS::SERVER> *websocket)
	: _websocket(websocket)
{
}

void WebsocketConnection::FrameComplete(uint64_t frame_id, const TcpProtocol &proto)
{
	(void) frame_id;
	if (!_firstFrameSent)
	{
		sendInitialData(proto);
		_firstFrameSent = true;
	}
}

void WebsocketConnection::LogMessage(uint64_t frame_id, const std::string &message)
{
	json j = { { "t", "Log" }, { "frame", frame_id }, { "msg", message} };
	sendString(j.dump());
}

void WebsocketConnection::sendInitialData(const TcpProtocol &proto)
{
	sendString(json(proto.GetGameInfo()).dump());
	sendString(json(*proto.MakeWorldUpdateMessage()).dump());
}

void WebsocketConnection::sendString(std::string data)
{
	_websocket->send(data.data(), data.length(), uWS::OpCode::TEXT);
}
