#pragma once

#include <vector>
#include <msgpack.hpp>
#include "types.h"

namespace MsgPackProtocol
{
	typedef enum
	{
		MESSAGE_TYPE_GAME_INFO = 0x00,
		MESSAGE_TYPE_WORLD_UPDATE = 0x01,

		MESSAGE_TYPE_TICK = 0x10,

		MESSAGE_TYPE_BOT_SPAWN = 0x20,
		MESSAGE_TYPE_BOT_KILL = 0x21,
		MESSAGE_TYPE_BOT_MOVE = 0x22,
		MESSAGE_TYPE_BOT_LOG = 0x23,
		MESSAGE_TYPE_BOT_STATS = 0x24,
		MESSAGE_TYPE_BOT_MOVE_HEAD = 0x25,

		MESSAGE_TYPE_FOOD_SPAWN = 0x30,
		MESSAGE_TYPE_FOOD_CONSUME = 0x31,
		MESSAGE_TYPE_FOOD_DECAY = 0x32,

		MESSAGE_TYPE_PLAYER_INFO = 0xF0,
	} MessageType;

	static constexpr const uint8_t PROTOCOL_VERSION = 1;

	struct FoodItem
	{
		guid_t guid;
		Vector2D position;
		real_t value;

		const Vector2D& pos() const { return position; }
	};

	struct SnakeSegmentItem
	{
		guid_t bot_id;
		Vector2D position;
		const Vector2D& pos() const { return position; }
	};

	struct BotItem
	{
		guid_t guid;
		std::string name;
		int database_id;
		uint32_t face_id;
		uint32_t dog_tag_id;
		std::vector<uint32_t> color;

		real_t mass;
		real_t segment_radius;
		std::vector<SnakeSegmentItem> segments;
	};

	struct BotLogItem
	{
		uint64_t viewer_key;
		std::string message;
	};

	struct BotMoveHeadItem
	{
		guid_t bot_id;
		double mass;
		// one head position for each step moved in this frame, in temporal order
		std::vector<Vector2D> new_head_positions;
	};

	struct BotStatsItem
	{
		guid_t bot_id;
		double natural_food_consumed;
		double carrion_food_consumed;
		double hunted_food_consumed;
		double mass;
	};

	struct Message
	{
		MessageType messageType;
		Message(MessageType type) : messageType(type) {}
		virtual ~Message() = default;
	};

	struct GameInfoMessage : public Message
	{
		double world_size_x = 0;
		double world_size_y = 0;
		double food_decay_per_frame = 0;
		double snake_distance_per_step = 0;
		double snake_segment_distance_factor = 0;
		double snake_segment_distance_exponent = 0;
		double snake_pull_factor = 0;
		GameInfoMessage(): Message(MESSAGE_TYPE_GAME_INFO) {}
	};

	struct PlayerInfoMessage : public Message
	{
		guid_t player_id; // id der von dieser Verbindung gesteuerten Schlange
		PlayerInfoMessage(): Message(MESSAGE_TYPE_PLAYER_INFO) {}
	};

	struct TickMessage : public Message
	{
		guid_t frame_id; // frame counter since start of server
		TickMessage(): Message(MESSAGE_TYPE_TICK) {}
	};

	struct WorldUpdateMessage : public Message
	{
		std::vector<BotItem> bots;
		std::vector<FoodItem> food;
		WorldUpdateMessage(): Message(MESSAGE_TYPE_WORLD_UPDATE) {}
	};

	struct BotSpawnMessage : public Message
	{
		BotItem bot;
		BotSpawnMessage(): Message(MESSAGE_TYPE_BOT_SPAWN) {}
	};

	struct BotMoveItem
	{
		guid_t bot_id;
		std::vector<SnakeSegmentItem> new_segments;
		size_t current_length;
		real_t current_segment_radius;
	};

	struct BotMoveMessage : public Message
	{
		std::vector<BotMoveItem> items;
		BotMoveMessage(): Message(MESSAGE_TYPE_BOT_MOVE) {}
	};

	struct BotKillMessage : public Message
	{
		guid_t killer_id;
		guid_t victim_id; // victim is deleted in this frame
		BotKillMessage(): Message(MESSAGE_TYPE_BOT_KILL) {}
	};

	struct BotLogMessage : public Message
	{
		std::vector<BotLogItem> items;
		BotLogMessage(): Message(MESSAGE_TYPE_BOT_LOG) {}
	};

	struct BotStatsMessage : public Message
	{
		std::vector<BotStatsItem> items;
		BotStatsMessage(): Message(MESSAGE_TYPE_BOT_STATS) {}
	};

	struct BotMoveHeadMessage : public Message
	{
		std::vector<BotMoveHeadItem> items;
		BotMoveHeadMessage(): Message(MESSAGE_TYPE_BOT_MOVE_HEAD) {}
	};

	struct FoodSpawnMessage : public Message
	{
		std::vector<FoodItem> new_food;
		FoodSpawnMessage(): Message(MESSAGE_TYPE_FOOD_SPAWN) {}
	};

	struct FoodConsumeItem
	{
		guid_t food_id; // food is deleted in this frame
		guid_t bot_id; // bot consuming the food
	};

	struct FoodConsumeMessage : public Message
	{
		std::vector<FoodConsumeItem> items;
		FoodConsumeMessage(): Message(MESSAGE_TYPE_FOOD_CONSUME) {}
	};

	struct FoodDecayMessage : public Message
	{
		std::vector<guid_t> food_ids; // food is deleted in this frame
		FoodDecayMessage(): Message(MESSAGE_TYPE_FOOD_DECAY) {}
	};

	void pack(msgpack::sbuffer& buf, const Message& msg);
}

namespace msgpack {
	MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
		namespace adaptor {

			template <> struct pack<MsgPackProtocol::GameInfoMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::GameInfoMessage const& v) const
				{
					o.pack_array(9);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_GAME_INFO));
					o.pack(v.world_size_x);
					o.pack(v.world_size_y);
					o.pack(v.food_decay_per_frame);
					o.pack(v.snake_distance_per_step);
					o.pack(v.snake_segment_distance_factor);
					o.pack(v.snake_segment_distance_exponent);
					o.pack(v.snake_pull_factor);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::GameInfoMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::GameInfoMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size < 5) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.world_size_x;
					o.via.array.ptr[3] >> v.world_size_y;
					o.via.array.ptr[4] >> v.food_decay_per_frame;
					if (o.via.array.size > 5)
					{
						o.via.array.ptr[5] >> v.snake_distance_per_step;
						o.via.array.ptr[6] >> v.snake_segment_distance_factor;
						o.via.array.ptr[7] >> v.snake_segment_distance_exponent;
						o.via.array.ptr[8] >> v.snake_pull_factor;
					}
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::PlayerInfoMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::PlayerInfoMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_PLAYER_INFO));
					o.pack(v.player_id);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::PlayerInfoMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::PlayerInfoMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.player_id;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::TickMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::TickMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_TICK));
					o.pack(v.frame_id);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::TickMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::TickMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.frame_id;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::WorldUpdateMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::WorldUpdateMessage const& v) const
				{
					o.pack_array(4);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_WORLD_UPDATE));
					o.pack(v.bots);
					o.pack(v.food);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::WorldUpdateMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::WorldUpdateMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 4) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.bots;
					o.via.array.ptr[3] >> v.food;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotSpawnMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotSpawnMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_BOT_SPAWN));
					o.pack(v.bot);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotSpawnMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotSpawnMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.bot;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotMoveMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotMoveMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_BOT_MOVE));
					o.pack(v.items);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotMoveMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotMoveMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.items;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotMoveItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotMoveItem const& v) const
				{
					o.pack_array(4);
					o.pack(v.bot_id);
					o.pack(v.new_segments);
					o.pack(v.current_length);
					o.pack(v.current_segment_radius);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotMoveItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotMoveItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 4) throw msgpack::type_error();
					o.via.array.ptr[0] >> v.bot_id;
					o.via.array.ptr[1] >> v.new_segments;
					o.via.array.ptr[2] >> v.current_length;
					o.via.array.ptr[3] >> v.current_segment_radius;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotMoveHeadMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotMoveHeadMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_BOT_MOVE_HEAD));
					o.pack(v.items);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotMoveHeadMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotMoveHeadMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.items;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotMoveHeadItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotMoveHeadItem const& v) const
				{
					o.pack_array(3);
					o.pack(v.bot_id);
					o.pack(v.mass);
					o.pack(v.new_head_positions);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotMoveHeadItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotMoveHeadItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[0] >> v.bot_id;
					o.via.array.ptr[1] >> v.mass;
					o.via.array.ptr[2] >> v.new_head_positions;
					return o;
				}
			};

			template <> struct pack<Vector2D>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, Vector2D const& v) const
				{
					o.pack_array(2);
					o.pack(v.x());
					o.pack(v.y());
					return o;
				}
			};

			template<> struct convert<Vector2D>
			{
				msgpack::object const& operator()(msgpack::object const& o, Vector2D& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 2) throw msgpack::type_error();
					v = {
						o.via.array.ptr[0].via.f64,
						o.via.array.ptr[1].via.f64
					};
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotKillMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotKillMessage const& v) const
				{
					o.pack_array(4);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_BOT_KILL));
					o.pack(v.killer_id);
					o.pack(v.victim_id);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotKillMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotKillMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 4) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.killer_id;
					o.via.array.ptr[3] >> v.victim_id;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotLogMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotLogMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_BOT_LOG));
					o.pack(v.items);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotLogMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotLogMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.items;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotLogItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotLogItem const& v) const
				{
					o.pack_array(2);
					o.pack(v.viewer_key);
					o.pack(v.message);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotLogItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotLogItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 2) throw msgpack::type_error();
					o.via.array.ptr[0] >> v.viewer_key;
					o.via.array.ptr[1] >> v.message;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotStatsMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotStatsMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_BOT_STATS));
					o.pack(v.items);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotStatsMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotStatsMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.items;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotStatsItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotStatsItem const& v) const
				{
					o.pack_array(5);
					o.pack(v.bot_id);
					o.pack(v.natural_food_consumed);
					o.pack(v.carrion_food_consumed);
					o.pack(v.hunted_food_consumed);
					o.pack(v.mass);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotStatsItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotStatsItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 5) throw msgpack::type_error();
					o.via.array.ptr[0] >> v.bot_id;
					o.via.array.ptr[1] >> v.natural_food_consumed;
					o.via.array.ptr[2] >> v.carrion_food_consumed;
					o.via.array.ptr[3] >> v.hunted_food_consumed;
					o.via.array.ptr[4] >> v.mass;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::FoodSpawnMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::FoodSpawnMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_FOOD_SPAWN));
					o.pack(v.new_food);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::FoodSpawnMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::FoodSpawnMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.new_food;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::FoodConsumeMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::FoodConsumeMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_FOOD_CONSUME));
					o.pack(v.items);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::FoodConsumeMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::FoodConsumeMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.items;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::FoodConsumeItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::FoodConsumeItem const& v) const
				{
					o.pack_array(2);
					o.pack(v.food_id);
					o.pack(v.bot_id);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::FoodConsumeItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::FoodConsumeItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 2) throw msgpack::type_error();
					o.via.array.ptr[0] >> v.food_id;
					o.via.array.ptr[1] >> v.bot_id;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::FoodDecayMessage>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::FoodDecayMessage const& v) const
				{
					o.pack_array(3);
					o.pack(MsgPackProtocol::PROTOCOL_VERSION);
					o.pack(static_cast<int>(MsgPackProtocol::MESSAGE_TYPE_FOOD_DECAY));
					o.pack(v.food_ids);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::FoodDecayMessage>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::FoodDecayMessage& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 3) throw msgpack::type_error();
					o.via.array.ptr[2] >> v.food_ids;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::SnakeSegmentItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::SnakeSegmentItem const& v) const
				{
					o.pack_array(2);
					o.pack(v.pos().x());
					o.pack(v.pos().y());
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::SnakeSegmentItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::SnakeSegmentItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 2) throw msgpack::type_error();
					v.bot_id = 0;
					v.position = {
						o.via.array.ptr[0].via.f64,
						o.via.array.ptr[1].via.f64,
					};
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::FoodItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::FoodItem const& v) const
				{
					o.pack_array(4);
					o.pack(v.guid);
					o.pack(v.pos().x());
					o.pack(v.pos().y());
					o.pack(v.value);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::FoodItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::FoodItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 4) throw msgpack::type_error();
					o.via.array.ptr[0] >> v.guid;
					v.position = {
						o.via.array.ptr[1].via.f64,
						o.via.array.ptr[2].via.f64,
					};
					o.via.array.ptr[3] >> v.value;
					return o;
				}
			};

			template <> struct pack<MsgPackProtocol::BotItem>
			{
				template <typename Stream> msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, MsgPackProtocol::BotItem const& v) const
				{
					o.pack_array(9);
					o.pack(v.guid);
					o.pack(v.name);
					o.pack(v.database_id);
					o.pack(v.face_id);
					o.pack(v.dog_tag_id);
					o.pack(v.color);
					o.pack(v.mass);
					o.pack(v.segment_radius);
					o.pack(v.segments);
					return o;
				}
			};

			template<> struct convert<MsgPackProtocol::BotItem>
			{
				msgpack::object const& operator()(msgpack::object const& o, MsgPackProtocol::BotItem& v) const
				{
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 9) throw msgpack::type_error();
					o.via.array.ptr[0] >> v.guid;
					o.via.array.ptr[1] >> v.name;
					o.via.array.ptr[2] >> v.database_id;
					o.via.array.ptr[3] >> v.face_id;
					o.via.array.ptr[4] >> v.dog_tag_id;
					o.via.array.ptr[5] >> v.color;
					o.via.array.ptr[6] >> v.mass;
					o.via.array.ptr[7] >> v.segment_radius;
					o.via.array.ptr[8] >> v.segments;
					for (auto &segmentItem: v.segments)
					{
						segmentItem.bot_id = v.guid;
					}
					return o;
				}
			};

		} // namespace adaptor
	} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack
