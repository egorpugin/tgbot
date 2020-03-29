#pragma once

#include "tgbot/Api.h"
#include "tgbot/EventHandler.h"

#define TGBOT_DEFAULT_UPDATE_LIMIT 100
#define TGBOT_DEFAULT_UPDATE_TIMEOUT 10

namespace TgBot
{

class EventBroadcaster;
class CurlHttpClient;

/// This object holds other objects specific for this bot instance.
class TGBOT_API Bot
{
public:
    explicit Bot(const std::string &token);
    ~Bot(); // deletes unique_ptr

    /// returns token for accessing api
    const std::string &getToken() const { return token; }

    /// returns object which can execute Telegram Bot API methods
    const Api &getApi() const { return api; }

    /// used for fine tune setup of http connections
    CurlHttpClient &getHttpClient() const { return *httpClient; }

    /// returns object which holds all event listeners
    EventBroadcaster &getEvents() { return *eventBroadcaster; }

    /// returns object which handles new update objects.
    /// Usually it's only needed for TgWebhookLocalServer and TgWebhookTcpServer objects
    const EventHandler &getEventHandler() const { return eventHandler; }

    /// returns last processed update id + 1
    /// Designed to be executed in a loop.
    Integer processUpdates(Integer offset = 0, Integer limit = TGBOT_DEFAULT_UPDATE_LIMIT, Integer timeout = TGBOT_DEFAULT_UPDATE_TIMEOUT, const Vector<String> &allowed_updates = {}) const;

    /// Calls processUpdates() in a loop.
    /// Starts long poll. After new update will come, this method will parse it
    /// and send to EventHandler which invokes your listeners.
    void longPoll(Integer limit = TGBOT_DEFAULT_UPDATE_LIMIT, Integer timeout = TGBOT_DEFAULT_UPDATE_TIMEOUT, const Vector<String> &allowed_updates = {}) const;

    const std::string &getBaseUrl() const { return base_url; }
    const std::string &getBaseFileUrl() const { return base_file_url; }

    int getDefaultTimeout() const { return 10; }

private:
    std::string token;
    std::unique_ptr<CurlHttpClient> httpClient;
    Api api;
    std::unique_ptr<EventBroadcaster> eventBroadcaster;
    EventHandler eventHandler;
    std::string base_url;
    std::string base_file_url;
};

}
