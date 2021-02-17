#pragma once

#include "tgbot/Api.h"

namespace tgbot {

class EventBroadcaster;
class CurlHttpClient;

/// This object holds other objects specific for this bot instance.
class TGBOT_API Bot
{
public:
    explicit Bot(const std::string &token);
    virtual ~Bot();

    /// returns token for accessing api
    const std::string &getToken() const { return token; }

    /// returns object which can execute Telegram Bot API methods
    const Api &getApi() const { return api; }

    /// used for fine tune setup of http connections
    CurlHttpClient &getHttpClient() const { return *httpClient; }

    /// returns last processed update id + 1
    /// Designed to be executed in a loop.
    Integer processUpdates(Integer offset = 0, Integer limit = default_update_limit, Integer timeout = default_update_timeout, const Vector<String> &allowed_updates = {});

    /// Calls processUpdates() in a loop.
    /// Starts long poll. After new update will come, this method will parse it
    /// and send to EventHandler which invokes your listeners.
    void longPoll(Integer limit = default_update_limit, Integer timeout = default_update_timeout, const Vector<String> &allowed_updates = {});

    const std::string &getBaseUrl() const { return base_url; }
    const std::string &getBaseFileUrl() const { return base_file_url; }

    int getDefaultTimeout() const { return 10; }

    std::string makeFileUrl(const std::string &file_path) const;

    virtual void handleUpdate(const Update &update) = 0;

private:
    std::string token;
    std::unique_ptr<CurlHttpClient> httpClient;
    Api api;
    std::string base_url;
    std::string base_file_url;

    static const int default_update_limit = 100;
    static const int default_update_timeout = 10;
};

}
