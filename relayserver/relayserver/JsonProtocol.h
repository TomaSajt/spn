#pragma once

#include "MsgPackProtocol.h"
#include <nlohmann/json.hpp>
using nlohmann::json;

namespace MsgPackProtocol
{
	void to_json(json& j, const Message& msg);

	void to_json(json& j, const GameInfoMessage& msg);
	void to_json(json& j, const WorldUpdateMessage& msg);
	void to_json(json& j, const BotItem& item);
	void to_json(json& j, const SnakeSegmentItem& item);
	void to_json(json& j, const FoodItem& item);
	void to_json(json& j, const TickMessage& msg);
	void to_json(json& j, const BotSpawnMessage& msg);
	void to_json(json& j, const BotKillMessage& msg);
	void to_json(json& j, const BotMoveMessage& msg);
	void to_json(json& j, const BotMoveItem& item);
	void to_json(json& j, const BotStatsMessage& msg);
	void to_json(json& j, const BotMoveHeadMessage& msg);
	void to_json(json& j, const BotMoveHeadItem& item);
	void to_json(json& j, const FoodSpawnMessage& msg);
	void to_json(json& j, const FoodConsumeMessage& msg);
	void to_json(json& j, const FoodConsumeItem& item);
	void to_json(json& j, const FoodDecayMessage& msg);
	void to_json(json& j, const PlayerInfoMessage& msg);
}
