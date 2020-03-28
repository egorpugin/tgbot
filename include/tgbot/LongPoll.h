#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace TgBot
{

class Api;
class Bot;
class EventHandler;

/**
 * @brief This class handles long polling and updates parsing.
 *
 * @ingroup net
 */
class TGBOT_API LongPoll
{
public:
    LongPoll(const Bot &bot, std::int32_t limit = 100, std::int32_t timeout = 10, const std::vector<std::string> &updates = {});

    /**
     * @brief Starts long poll. After new update will come, this method will parse it
     * and send to EventHandler which invokes your listeners. Designed to be executed in a loop.
     */
    void start();

private:
    const Api &api;
    const EventHandler &eventHandler;
    std::int32_t lastUpdateId = 0;
    std::int32_t limit;
    std::int32_t timeout;
    std::vector<std::string> allowUpdates;
};

}
