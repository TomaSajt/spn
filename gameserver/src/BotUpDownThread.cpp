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

#include <chrono>

#include "Bot.h"

#include "config.h"

#include "BotUpDownThread.h"

#include <pthread.h>

using namespace std::chrono_literals;

BotUpDownThread::BotUpDownThread(void)
	: m_startupThreads(config::NTHREADS_BOT_STARTUP)
{
	for(auto &thread : m_startupThreads) {
		thread = std::thread(
				[this] ()
				{
					while(!m_shutdown) {
						bool idle = true;

						// check for work in startup queue
						std::shared_ptr<Bot> bot;

						{
							std::lock_guard<std::mutex> guard(m_startupQueueMutex);
							if(!m_startupInQueue.empty()) {
								bot = m_startupInQueue.front();
								m_startupInQueue.pop();
							} else {
								bot = NULL;
							}
						}

						if(bot) {
							std::unique_ptr<Result> result(new Result{bot, "", true});

							try {
								bot->internalStartup();
							} catch(std::runtime_error &e) {
								result->message = e.what();
								result->success = false;
							}

							std::lock_guard<std::mutex> guard(m_startupQueueMutex);
							m_startupOutQueue.push(std::move(result));

							if(!m_startupInQueue.empty()) {
								idle = false;
							}
						}

						if(idle) {
							// no work remains in queue -> sleep
							std::this_thread::sleep_for(1s);
						}
					}
				});
	}

	m_shutdownThread = std::thread(
			[this] ()
			{
				while(!m_shutdown) {
					bool idle = true;

					// check for work in shutdown queue
					std::shared_ptr<Bot> bot;

					{
						std::lock_guard<std::mutex> guard(m_shutdownQueueMutex);
						if(!m_shutdownInQueue.empty()) {
							bot = m_shutdownInQueue.front();
							m_shutdownInQueue.pop();
						} else {
							bot = NULL;
						}
					}

					if(bot) {
						std::unique_ptr<Result> result(new Result{bot, "", true});

						try {
							bot->internalShutdown();
						} catch(std::runtime_error &e) {
							result->message = e.what();
							result->success = false;
						}

						std::lock_guard<std::mutex> guard(m_shutdownQueueMutex);
						m_shutdownOutQueue.push(std::move(result));

						if(!m_shutdownInQueue.empty()) {
							idle = false;
						}
					}

					if(idle) {
						// no work remains in queue -> sleep
						std::this_thread::sleep_for(1s);
					}
				}
			});

	// remove this code if it does not compile on your system. It does not affect
	// the program's functionality.
	for(size_t i = 0; i < m_startupThreads.size(); i++) {
		std::ostringstream nameBuilder;
		nameBuilder << "bot_startup" << i;
		pthread_setname_np(m_startupThreads[i].native_handle(), nameBuilder.str().c_str());
	}

	pthread_setname_np(m_shutdownThread.native_handle(), "bot_shutdown");
}

BotUpDownThread::~BotUpDownThread()
{
	// request thread shutdown
	m_shutdown = true;

	for(auto &thread: m_startupThreads) {
		thread.join();
	}

	m_shutdownThread.join();
}

void BotUpDownThread::addStartupBot(const std::shared_ptr<Bot> &bot)
{
	m_dbIdsInProgress.insert(bot->getDatabaseId());

	std::lock_guard<std::mutex> guard(m_startupQueueMutex);
	m_startupInQueue.push(bot);

	std::cout << "Startup input queue length: " << m_startupInQueue.size() << std::endl;
}

void BotUpDownThread::addShutdownBot(const std::shared_ptr<Bot> &bot)
{
	m_dbIdsInProgress.insert(bot->getDatabaseId());

	std::lock_guard<std::mutex> guard(m_shutdownQueueMutex);
	m_shutdownInQueue.push(bot);

	std::cout << "Shutdown input queue length: " << m_shutdownInQueue.size() << std::endl;
}

std::unique_ptr<BotUpDownThread::Result> BotUpDownThread::getStartupResult(void)
{
	std::lock_guard<std::mutex> guard(m_startupQueueMutex);

	if(m_startupOutQueue.empty()) {
		return NULL;
	}

	std::unique_ptr<Result> result(std::move(m_startupOutQueue.front()));
	m_startupOutQueue.pop();

	m_dbIdsInProgress.erase(result->bot->getDatabaseId());
	return result;
}

std::unique_ptr<BotUpDownThread::Result> BotUpDownThread::getShutDownResult(void)
{
	std::lock_guard<std::mutex> guard(m_shutdownQueueMutex);

	if(m_shutdownOutQueue.empty()) {
		return NULL;
	}

	std::unique_ptr<Result> result(std::move(m_shutdownOutQueue.front()));
	m_shutdownOutQueue.pop();

	m_dbIdsInProgress.erase(result->bot->getDatabaseId());
	return result;
}

size_t BotUpDownThread::getStartupQueueLen(void)
{
	std::lock_guard<std::mutex> guard(m_startupQueueMutex);
	return m_startupInQueue.size();
}

size_t BotUpDownThread::getShutdownQueueLen(void)
{
	std::lock_guard<std::mutex> guard(m_shutdownQueueMutex);
	return m_shutdownInQueue.size();
}
