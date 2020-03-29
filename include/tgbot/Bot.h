#pragma once

#include "tgbot/Api.h"
#include "tgbot/EventHandler.h"

namespace TgBot
{

class EventBroadcaster;
class HttpClient;

/// This object holds other objects specific for this bot instance.
class TGBOT_API Bot
{
public:
    explicit Bot(const std::string &token, const HttpClient &httpClient = getDefaultHttpClient());
    ~Bot(); // deletes unique_ptr

    /// returns token for accessing api
    const std::string &getToken() const { return token; }

    /// returns object which can execute Telegram Bot API methods
    const Api &getApi() const { return api; }

    /// returns object which holds all event listeners
    EventBroadcaster &getEvents() { return *eventBroadcaster; }

    /// returns object which handles new update objects.
    /// Usually it's only needed for TgWebhookLocalServer and TgWebhookTcpServer objects
    const EventHandler &getEventHandler() const { return eventHandler; }

    /// returns last processed update id + 1
    /// Designed to be executed in a loop.
    Integer processUpdates(Integer offset = 0, Integer limit = 100, Integer timeout = 20, const Vector<String> &allowed_updates = {}) const;

    /// Calls getUpdates() in a loop.
    /// Starts long poll. After new update will come, this method will parse it
    /// and send to EventHandler which invokes your listeners.
    void longPoll(Integer limit = 100, Integer timeout = 20, const Vector<String> &allowed_updates = {}) const;

private:
    std::string token;
    Api api;
    std::unique_ptr<EventBroadcaster> eventBroadcaster;
    EventHandler eventHandler;

    static HttpClient &getDefaultHttpClient();
};

}
