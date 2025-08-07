#include "MsgPackProtocol.h"

void MsgPackProtocol::pack(msgpack::sbuffer &buf, const Message &msg)
{
	switch (msg.messageType)
	{
		case MESSAGE_TYPE_GAME_INFO:
			msgpack::pack(buf, *static_cast<const GameInfoMessage*>(&msg));
			break;
		case MESSAGE_TYPE_WORLD_UPDATE:
			msgpack::pack(buf, *static_cast<const WorldUpdateMessage*>(&msg));
			break;
		case MESSAGE_TYPE_TICK:
			msgpack::pack(buf, *static_cast<const TickMessage*>(&msg));
			break;
		case MESSAGE_TYPE_BOT_SPAWN:
			msgpack::pack(buf, *static_cast<const BotSpawnMessage*>(&msg));
			break;
		case MESSAGE_TYPE_BOT_KILL:
			msgpack::pack(buf, *static_cast<const BotKillMessage*>(&msg));
			break;
		case MESSAGE_TYPE_BOT_MOVE:
			msgpack::pack(buf, *static_cast<const BotMoveMessage*>(&msg));
			break;
		case MESSAGE_TYPE_BOT_MOVE_HEAD:
			msgpack::pack(buf, *static_cast<const BotMoveHeadMessage*>(&msg));
			break;
		case MESSAGE_TYPE_BOT_STATS:
			msgpack::pack(buf, *static_cast<const BotStatsMessage*>(&msg));
			break;
		case MESSAGE_TYPE_BOT_LOG:
			msgpack::pack(buf, *static_cast<const BotLogMessage*>(&msg));
			break;
		case MESSAGE_TYPE_FOOD_SPAWN:
			msgpack::pack(buf, *static_cast<const FoodSpawnMessage*>(&msg));
			break;
		case MESSAGE_TYPE_FOOD_CONSUME:
			msgpack::pack(buf, *static_cast<const FoodConsumeMessage*>(&msg));
			break;
		case MESSAGE_TYPE_FOOD_DECAY:
			msgpack::pack(buf, *static_cast<const FoodDecayMessage*>(&msg));
			break;
		case MESSAGE_TYPE_PLAYER_INFO:
			msgpack::pack(buf, *static_cast<const PlayerInfoMessage*>(&msg));
			break;
	}
}
