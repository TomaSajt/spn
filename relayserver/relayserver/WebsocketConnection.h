#pragma once

#include <uWS.h>

class TcpProtocol;

class WebsocketConnection
{
	public:
		WebsocketConnection(uWS::WebSocket<uWS::SERVER> *websocket);
		void FrameComplete(uint64_t frame_id, const TcpProtocol& proto);
		void LogMessage(uint64_t frame_id, const std::string& message);
		void sendString(std::string data);
		uint64_t getViewerKey() { return _viewerKey; }
		void setViewerKey(uint64_t key) { _viewerKey = key; }

	private:
		uWS::WebSocket<uWS::SERVER> *_websocket;
		bool _firstFrameSent = false;
		uint64_t _viewerKey = 0;

		void sendInitialData(const TcpProtocol& proto);

};
