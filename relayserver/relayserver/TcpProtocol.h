#pragma once
#include <stddef.h>
#include <stdint.h>
#include <functional>
#include <vector>
#include <memory>
#include <map>
#include "MsgPackProtocol.h"

using BotItem = MsgPackProtocol::BotItem;
using FoodItem = MsgPackProtocol::FoodItem;
using SnakeSegmentItem = MsgPackProtocol::SnakeSegmentItem;

class TcpProtocol
{
	public:
		typedef std::function<void(uint64_t frame_id)> FrameCompleteCallback;
		typedef std::function<void(const MsgPackProtocol::BotStatsMessage& msg)> StatsReceivedCallback;
		static constexpr const size_t BUFFER_SIZE = 1024*1024;

		TcpProtocol();
		void SetFrameCompleteCallback(FrameCompleteCallback callback);
		void SetStatsReceivedCallback(StatsReceivedCallback callback);
		bool Read(int socket);

		const MsgPackProtocol::GameInfoMessage& GetGameInfo() const { return _gameInfo; }

		std::unique_ptr<MsgPackProtocol::WorldUpdateMessage> MakeWorldUpdateMessage() const;

		const std::vector<std::unique_ptr<MsgPackProtocol::Message>>& GetPendingMessages() const { return _pendingMessages; }
		typedef std::map<uint64_t, std::vector<MsgPackProtocol::BotLogItem>> LogItemMap;
		const LogItemMap& GetPendingLogItems() const { return _pendingLogItems; }
		void ClearLogItems();

	private:
		std::vector<char> _buf;
		size_t _bufHead=0;
		size_t _bufTail=0;

		FrameCompleteCallback _frameCompleteCallback;
		StatsReceivedCallback _statsReceivedCallback;
		MsgPackProtocol::GameInfoMessage _gameInfo;
		MsgPackProtocol::BotStatsMessage _botStats;
		std::map<guid_t,FoodItem> _foodMap;
		std::map<guid_t,BotItem> _botsMap;
		std::vector<std::unique_ptr<MsgPackProtocol::Message>> _pendingMessages;

		LogItemMap _pendingLogItems;

		void OnMessageReceived(const char *data, size_t count);

		void OnGameInfoReceived(const MsgPackProtocol::GameInfoMessage& msg);
		void OnWorldUpdateReceived(const MsgPackProtocol::WorldUpdateMessage& msg);
		void OnTickReceived(const MsgPackProtocol::TickMessage &msg);

		void OnFoodSpawnReceived(const MsgPackProtocol::FoodSpawnMessage& msg);
		void OnFoodConsumedReceived(const MsgPackProtocol::FoodConsumeMessage& msg);
		void OnFoodDecayedReceived(const MsgPackProtocol::FoodDecayMessage& msg);

		void OnBotSpawnReceived(const MsgPackProtocol::BotSpawnMessage& msg);
		void OnBotKillReceived(const MsgPackProtocol::BotKillMessage &msg);
		void OnBotMoveReceived(std::unique_ptr<MsgPackProtocol::BotMoveMessage> msg);
		void OnBotLogReceived(std::unique_ptr<MsgPackProtocol::BotLogMessage> msg);
		void OnBotStatsReceived(std::unique_ptr<MsgPackProtocol::BotStatsMessage> msg);
		void OnBotMoveHeadReceived(std::unique_ptr<MsgPackProtocol::BotMoveHeadMessage> msg);
};
