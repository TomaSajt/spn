/*
 * Schlangenprogrammiernacht: A programming game for GPN18.
 * Copyright (C) 2018  bytewerk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>

#include "Game.h"
#include "config.h"
#include "Environment.h"
#include "debug_funcs.h"
#include "MsgPackUpdateTracker.h"
#include "Stopwatch.h"

Game::Game()
{
	m_field = std::make_unique<Field>(
		config::FIELD_SIZE_X, config::FIELD_SIZE_Y,
		config::FIELD_STATIC_FOOD,
		std::make_unique<MsgPackUpdateTracker>()
	);

	server.AddConnectionEstablishedListener(
		[this](TcpSocket& socket)
		{
			return OnConnectionEstablished(socket);
		}
	);

	server.AddConnectionClosedListener(
		[this](TcpSocket& socket)
		{
			return OnConnectionClosed(socket);
		}
	);

	server.AddDataAvailableListener(
		[this](TcpSocket& socket)
		{
			return OnDataAvailable(socket);
		}
	);

	m_field->addBotKilledCallback(
		[this](std::shared_ptr<Bot> victim, std::shared_ptr<Bot> killer)
		{
			long killer_id = (killer==nullptr) ? -1 : killer->getDatabaseId();
			m_database->ReportBotKilled(
				victim->getDatabaseId(),
				victim->getDatabaseVersionId(),
				victim->getStartFrame(),
				m_field->getCurrentFrame(),
				killer_id,
				victim->getSnake()->getMaximumMass(),
				victim->getSnake()->getMass(),
				victim->getConsumedNaturalFood(),
				victim->getConsumedFoodHuntedByOthers(),
				victim->getConsumedFoodHuntedBySelf()
			);

			m_database->UpdatePersistentData(
				victim->getDatabaseId(),
				victim->getPersistentData()
			);

			// victim will be respawned on next database query
		}
	);

	m_field->addBotErrorCallback(
		[this](const std::shared_ptr<Bot> &failedBot, const std::string &errorMessage)
		{
			// Set the bot to crashed state in the database
			m_database->SetBotToCrashedState(failedBot->getDatabaseVersionId(), errorMessage);
		}
	);
}

bool Game::OnConnectionEstablished(TcpSocket &socket)
{
	std::cerr << "connection established to " << socket.GetPeer() << std::endl;

	// send initial state
	MsgPackUpdateTracker initTracker;
	initTracker.gameInfo();
	initTracker.worldState(*m_field);
	socket.Write(initTracker.serialize());

	socket.SetWriteBlocking(false);

	return true;
}

bool Game::OnConnectionClosed(TcpSocket &socket)
{
	std::cerr << "connection to " << socket.GetPeer() << " closed." << std::endl;

	return true;
}

bool Game::OnDataAvailable(TcpSocket &socket)
{
	char data[1024];
	ssize_t count = socket.Read(data, sizeof(data));
	if(count > 0) {
		// return to sender
		socket.Write(data, count, false);
	}
	return true;
}

void Game::ProcessOneFrame()
{
	// do all the game logic here and send updates to clients
	auto frame = m_field->getCurrentFrame();

	double now = getCurrentTimestamp();

	Stopwatch swProcessFrame("ProcessFrame");

	Stopwatch swDecayFood("DecayFood");
	m_field->decayFood();
	swDecayFood.Stop();

	Stopwatch swConsumeFood("ConsumeFood");
	m_field->consumeFood();
	swConsumeFood.Stop();

	Stopwatch swRemoveFood("RemoveFood");
	m_field->removeFood();
	swRemoveFood.Stop();

	Stopwatch swMoveAllBots("MoveAllBots");
	m_field->moveAllBots();
	swMoveAllBots.Stop();

	Stopwatch swProcessStats("ProcessStats");
	if(now > m_nextStreamStatsUpdateTime) {
		m_field->sendStatsToStream();
		m_nextStreamStatsUpdateTime = now + STREAM_STATS_UPDATE_INTERVAL;
	}
	if(now > m_nextDbStatsUpdateTime) {
		updateDbStats(now);
		m_nextDbStatsUpdateTime = now + DB_STATS_UPDATE_INTERVAL;
	}
	swProcessStats.Stop();

	Stopwatch swProcessLog("ProcessLog");
	m_field->processLog();
	swProcessLog.Stop();

	Stopwatch swProcessTick("ProcessTick");
	m_field->tick();
	swProcessTick.Stop();

	Stopwatch swLimbo("Limbo");
	m_field->updateLimbo();
	swLimbo.Stop();

	Stopwatch swSendUpdate("SendUpdate");
	// send differential update to all connected clients
	std::string update = m_field->getUpdateTracker().serialize();
	server.Broadcast(update);
	swSendUpdate.Stop();

	Stopwatch swQueryDB("QueryDB");
	if (now > m_nextDbQueryTime)
	{
		queryDB();
		m_nextDbQueryTime = now + DB_QUERY_INTERVAL;
	}
	swQueryDB.Stop();

	swProcessFrame.Stop();

#if DEBUG_TIMINGS
	std::cout << std::endl;
	std::cout << "Frame " << frame << " timings: " << std::endl;
	swDecayFood.Print();
	swConsumeFood.Print();
	swRemoveFood.Print();
	swMoveAllBots.Print();
	swProcessStats.Print();
	swProcessLog.Print();
	swProcessTick.Print();
	swSendUpdate.Print();
	swLimbo.Print();
	swQueryDB.Print();
	swProcessFrame.Print();
	std::cout << std::endl;
#endif
}

int Game::Main()
{
	// set up umask so we can create shared files for the bots
	umask(0000);

	if (!server.Listen(9010)) 
	{
		return -1;
	}

	if (!connectDB())
	{
		return -2;
	}

	for (auto id: m_database->GetActiveBotIds())
	{
		createBot(id);
	}

	// initialize framerate limiter
	m_nextFrameTime = getCurrentTimestamp();

	while(true)
	{
		ProcessOneFrame();
		server.Poll(0);

		waitForNextFrame();

		if(m_shuttingDown) {
			if(m_field->getBots().empty()) {
				std::cerr << "No more bots remaining, terminating main loop." << std::endl;
				break;
			} else {
				std::cerr << "Shutdown in progress: " << m_field->getBots().size() << " bots remaining." << std::endl;
			}
		}
	}

	return 0;
}

void Game::waitForNextFrame(void)
{
	static const constexpr double FRAME_TIME = 1.0/FPS;

	struct timespec nextFrameTS;
	nextFrameTS.tv_sec  = static_cast<time_t>(m_nextFrameTime);
	nextFrameTS.tv_nsec = static_cast<long  >((m_nextFrameTime - nextFrameTS.tv_sec) * 1e9);

	int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nextFrameTS, NULL);
	if(ret == -1) {
		std::cerr << "clock_nanosleep(CLOCK_MONOTONIC) failed: " << strerror(errno) << std::endl;
	}

	m_nextFrameTime += FRAME_TIME;
}

double Game::getCurrentTimestamp(void)
{
	struct timespec t;
	int ret = clock_gettime(CLOCK_MONOTONIC, &t);
	if(ret == -1) {
		std::cerr << "clock_gettime(CLOCK_MONOTONIC) failed: " << strerror(errno) << std::endl;
		return -1;
	}

	return t.tv_sec + t.tv_nsec * 1e-9;
}

bool Game::connectDB()
{
	auto db = std::make_unique<db::MysqlDatabase>();
	db->Connect(
		Environment::GetDefault(Environment::ENV_MYSQL_HOST, Environment::ENV_MYSQL_HOST_DEFAULT),
		Environment::GetDefault(Environment::ENV_MYSQL_USER, Environment::ENV_MYSQL_USER_DEFAULT),
		Environment::GetDefault(Environment::ENV_MYSQL_PASSWORD, Environment::ENV_MYSQL_PASSWORD_DEFAULT),
		Environment::GetDefault(Environment::ENV_MYSQL_DB, Environment::ENV_MYSQL_DB_DEFAULT)
	);
	m_database = std::move(db);
	return true;
}

void Game::queryDB()
{
	std::vector<int> active_ids;

	// skip database query on shutdown which causes all bots to commit suicide ]:->
	if(!m_shuttingDown)
	{
		active_ids = m_database->GetActiveBotIds();
		for (auto id: active_ids)
		{
			if (!m_field->isDatabaseIdActive(id))
			{
				createBot(id);
			}
		}
	}

	std::vector<std::shared_ptr<Bot>> kill_bots;
	for (auto& bot: m_field->getBots())
	{
		if (std::find(active_ids.begin(), active_ids.end(), bot->getDatabaseId()) == active_ids.end())
		{
			kill_bots.push_back(bot);
		}
	}

	for (auto& bot: kill_bots)
	{
		m_field->killBot(bot, bot); // suicide!
	}

	for (auto& cmd: m_database->GetActiveCommands())
	{
		if (cmd.command == db::Command::CMD_KILL)
		{
			auto bot = m_field->getBotByDatabaseId(static_cast<int>(cmd.bot_id));
			if (bot != nullptr)
			{
				m_field->killBot(bot, bot); // suicide!
				m_database->SetCommandCompleted(cmd.id, true, "killed");
			}
			else
			{
				m_database->SetCommandCompleted(cmd.id, false, "bot not known / not active");
			}
		}
		else
		{
			m_database->SetCommandCompleted(cmd.id, false, "command not known");
		}
	}
}

void Game::createBot(int bot_id)
{
	auto data = m_database->GetBotData(bot_id);
	if (data == nullptr)
	{
		return;
	}

	if(data->compile_state != "successful") {
		return;
	}

	std::string initErrorMessage;
	auto newBot = m_field->newBot(std::move(data), initErrorMessage);
	if (!initErrorMessage.empty())
	{
		m_database->SetBotToCrashedState(newBot->getDatabaseVersionId(), initErrorMessage);
	}
}

void Game::updateDbStats(double now)
{
	double fps;
	uint64_t current_frame;
	uint32_t running_bots;
	size_t start_queue_len;
	size_t stop_queue_len;
	double living_mass;
	double dead_mass;

	/*
	 * Gather statistics
	 */

	current_frame = m_field->getCurrentFrame();

	if(m_lastFPSUpdateTime != 0) {
		fps = (current_frame - m_lastFPSUpdateFrameCount) /
			(now - m_lastFPSUpdateTime);
	} else {
		// first update
		fps = -1;
	}

	m_lastFPSUpdateTime = now;
	m_lastFPSUpdateFrameCount = current_frame;

	running_bots = m_field->getBots().size();

	m_field->calculateCurrentMass(&living_mass, &dead_mass);
	m_field->getLimboStats(&start_queue_len, &stop_queue_len);

	/*
	 * Save to database
	 */
	m_database->UpdateLiveStats(fps, current_frame, running_bots, start_queue_len, stop_queue_len, living_mass, dead_mass);
}

void Game::Shutdown(void)
{
	m_shuttingDown = true;
}
