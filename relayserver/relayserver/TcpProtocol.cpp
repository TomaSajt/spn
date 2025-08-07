#include "TcpProtocol.h"
#include <stdint.h>
#include <unistd.h>
#include <array>
#include <algorithm>
#include <msgpack.hpp>
#include <iostream>

TcpProtocol::TcpProtocol()
{
	_buf.resize(BUFFER_SIZE);
}

void TcpProtocol::SetFrameCompleteCallback(TcpProtocol::FrameCompleteCallback callback)
{
	_frameCompleteCallback = callback;
}

void TcpProtocol::SetStatsReceivedCallback(StatsReceivedCallback callback)
{
	_statsReceivedCallback = callback;
}

bool TcpProtocol::Read(int socket)
{	
	ssize_t bytesRead = read(socket, &_buf[_bufTail], _buf.size()-_bufTail);

	if (bytesRead<=0) { return false; }
	_bufTail += static_cast<size_t>(bytesRead);

	while ((_bufTail - _bufHead)>=4)
	{
		size_t size = 4 + ntohl(*(reinterpret_cast<uint32_t*>(&_buf[_bufHead])));
		if (size > _buf.size()) { return false; }
		if (size <= (_bufTail - _bufHead))
		{
			OnMessageReceived(&_buf[_bufHead+4], size-4);
			_bufHead += size;
		}
		else
		{
			break;
		}
	}

	std::copy(_buf.begin()+_bufHead, _buf.begin()+_bufTail, _buf.begin());
	_bufTail -= _bufHead;
	_bufHead = 0;
	return true;
}

std::unique_ptr<MsgPackProtocol::WorldUpdateMessage> TcpProtocol::MakeWorldUpdateMessage() const
{
	auto result = std::make_unique<MsgPackProtocol::WorldUpdateMessage>();
	auto& food = result->food;
	auto& bots = result->bots;

	food.reserve(_foodMap.size());
	for (auto& kvp: _foodMap) {
		food.emplace_back(kvp.second);
	}

	bots.reserve(_botsMap.size());
	for (auto& kvp: _botsMap) {
		bots.emplace_back(kvp.second);
	}

	return result;
}

void TcpProtocol::ClearLogItems()
{
	for (auto &kvp: _pendingLogItems)
	{
		_pendingLogItems[kvp.first].clear();
	}
}

void TcpProtocol::OnMessageReceived(const char* data, size_t count)
{
	msgpack::object_handle obj;
	uint64_t version, message_type;

	msgpack::unpack(obj, data, count);
	if (obj.get().type != msgpack::type::ARRAY) { return; }

	auto arr = obj.get().via.array;
	if (arr.size<2) { return; }
	arr.ptr[0] >> version;
	arr.ptr[1] >> message_type;

	switch (message_type)
	{
		case MsgPackProtocol::MESSAGE_TYPE_GAME_INFO:
			OnGameInfoReceived(obj.get().as<MsgPackProtocol::GameInfoMessage>());
			break;

		case MsgPackProtocol::MESSAGE_TYPE_WORLD_UPDATE:
			OnWorldUpdateReceived(obj.get().as<MsgPackProtocol::WorldUpdateMessage>());
			break;

		case MsgPackProtocol::MESSAGE_TYPE_TICK:
		{
			OnTickReceived(obj.get().as<MsgPackProtocol::TickMessage>());
			break;
		}

		case MsgPackProtocol::MESSAGE_TYPE_BOT_SPAWN:
			OnBotSpawnReceived(obj.get().as<MsgPackProtocol::BotSpawnMessage>());
			break;

		case MsgPackProtocol::MESSAGE_TYPE_BOT_KILL:
			OnBotKillReceived(obj.get().as<MsgPackProtocol::BotKillMessage>());
			break;

		case MsgPackProtocol::MESSAGE_TYPE_BOT_MOVE:
		{
			auto msg = std::make_unique<MsgPackProtocol::BotMoveMessage>();
			obj.get().convert(*msg);
			OnBotMoveReceived(std::move(msg));
			break;
		}

		case MsgPackProtocol::MESSAGE_TYPE_BOT_STATS:
		{
			auto msg = std::make_unique<MsgPackProtocol::BotStatsMessage>();
			obj.get().convert(*msg);
			OnBotStatsReceived(std::move(msg));
			break;
		}

		case MsgPackProtocol::MESSAGE_TYPE_BOT_MOVE_HEAD:
		{
			auto msg = std::make_unique<MsgPackProtocol::BotMoveHeadMessage>();
			obj.get().convert(*msg);
			OnBotMoveHeadReceived(std::move(msg));
			break;
		}

		case MsgPackProtocol::MESSAGE_TYPE_BOT_LOG:
		{
			auto msg = std::make_unique<MsgPackProtocol::BotLogMessage>();
			obj.get().convert(*msg);
			OnBotLogReceived(std::move(msg));
			break;
		}

		case MsgPackProtocol::MESSAGE_TYPE_FOOD_SPAWN:
			OnFoodSpawnReceived(obj.get().as<MsgPackProtocol::FoodSpawnMessage>());
			break;

		case MsgPackProtocol::MESSAGE_TYPE_FOOD_CONSUME:
			OnFoodConsumedReceived(obj.get().as<MsgPackProtocol::FoodConsumeMessage>());
			break;

		case MsgPackProtocol::MESSAGE_TYPE_FOOD_DECAY:
			OnFoodDecayedReceived(obj.get().as<MsgPackProtocol::FoodDecayMessage>());
			break;
	}
}

void TcpProtocol::OnGameInfoReceived(const MsgPackProtocol::GameInfoMessage& msg)
{
	_gameInfo = msg;
}

void TcpProtocol::OnWorldUpdateReceived(const MsgPackProtocol::WorldUpdateMessage &msg)
{
	_botsMap.clear();
	for (auto& bot: msg.bots)
	{
		_botsMap.insert(std::make_pair(bot.guid, bot));
	}

	_foodMap.clear();
	for (auto& food: msg.food)
	{
		_foodMap.insert(std::make_pair(food.guid, food));
	}
}

void TcpProtocol::OnTickReceived(const MsgPackProtocol::TickMessage& msg)
{
	_pendingMessages.push_back(std::make_unique<MsgPackProtocol::TickMessage>(msg));
	if (_frameCompleteCallback!=nullptr)
	{
		_frameCompleteCallback(msg.frame_id);
	}
	_pendingMessages.clear();
}

void TcpProtocol::OnFoodSpawnReceived(const MsgPackProtocol::FoodSpawnMessage& msg)
{
	_pendingMessages.push_back(std::make_unique<MsgPackProtocol::FoodSpawnMessage>(msg));
	for (auto& item: msg.new_food)
	{
		_foodMap.insert(std::make_pair(item.guid, item));
	}
}

void TcpProtocol::OnFoodConsumedReceived(const MsgPackProtocol::FoodConsumeMessage &msg)
{
	_pendingMessages.push_back(std::make_unique<MsgPackProtocol::FoodConsumeMessage>(msg));
	for (auto& item: msg.items)
	{
		_foodMap.erase(item.food_id);
	}
}

void TcpProtocol::OnFoodDecayedReceived(const MsgPackProtocol::FoodDecayMessage &msg)
{
	_pendingMessages.push_back(std::make_unique<MsgPackProtocol::FoodDecayMessage>(msg));
	for (auto& id: msg.food_ids)
	{
		_foodMap.erase(id);
	}
}

void TcpProtocol::OnBotSpawnReceived(const MsgPackProtocol::BotSpawnMessage &msg)
{
	_pendingMessages.push_back(std::make_unique<MsgPackProtocol::BotSpawnMessage>(msg));
	auto result = _botsMap.insert(std::make_pair(msg.bot.guid, msg.bot));
	(result.first)->second.segments.reserve(100);
}

void TcpProtocol::OnBotKillReceived(const MsgPackProtocol::BotKillMessage& msg)
{
	_pendingMessages.push_back(std::make_unique<MsgPackProtocol::BotKillMessage>(msg));
	_botsMap.erase(msg.victim_id);
}

void TcpProtocol::OnBotMoveReceived(std::unique_ptr<MsgPackProtocol::BotMoveMessage> msg)
{
	for (auto& item: msg->items)
	{
		auto it = _botsMap.find(item.bot_id);
		if (it != _botsMap.end())
		{
			auto& bot = it->second;
			bot.segments.reserve(bot.segments.size() + item.new_segments.size());
			bot.segments.insert(bot.segments.begin(), item.new_segments.begin(), item.new_segments.end());
			bot.segments.resize(item.current_length);
			bot.segment_radius = item.current_segment_radius;
		}

	}
	//_pendingMessages.push_back(std::move(msg));
}

void TcpProtocol::OnBotLogReceived(std::unique_ptr<MsgPackProtocol::BotLogMessage> msg)
{
	for (auto& item: msg->items)
	{
		_pendingLogItems[item.viewer_key].emplace_back(item);
	}
}

void TcpProtocol::OnBotStatsReceived(std::unique_ptr<MsgPackProtocol::BotStatsMessage> msg)
{
	_botStats = *msg;
	if (_statsReceivedCallback!=nullptr)
	{
		_statsReceivedCallback(_botStats);
	}
	_pendingMessages.push_back(std::move(msg));
}

void TcpProtocol::OnBotMoveHeadReceived(std::unique_ptr<MsgPackProtocol::BotMoveHeadMessage> msg)
{
	_pendingMessages.push_back(std::move(msg));
}
