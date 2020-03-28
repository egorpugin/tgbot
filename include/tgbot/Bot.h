#pragma once

#include "tgbot/Api.h"
#include "tgbot/EventHandler.h"

#include <memory>
#include <string>

namespace TgBot
{

class EventBroadcaster;
class HttpClient;

/**
 * @brief This object holds other objects specific for this bot instance.
 *
 * @ingroup general
 */
class TGBOT_API Bot
{
public:
    explicit Bot(const std::string &token, const HttpClient &httpClient = getDefaultHttpClient());
    ~Bot(); // deletes unique_ptr

    /// returns token for accessing api
    const std::string &getToken() const {
        return token;
    }

    /// returns object which can execute Telegram Bot API methods
    const Api &getApi() const {
        return api;
    }

    /// returns object which holds all event listeners
    EventBroadcaster &getEvents() {
        return *eventBroadcaster;
    }

    /// returns object which handles new update objects.
    /// Usually it's only needed for TgLongPoll, TgWebhookLocalServer and TgWebhookTcpServer objects
    const EventHandler &getEventHandler() const {
        return eventHandler;
    }

private:
    const std::string token;
    const Api api;
    std::unique_ptr<EventBroadcaster> eventBroadcaster;
    const EventHandler eventHandler;

    static HttpClient &getDefaultHttpClient();
};

}
