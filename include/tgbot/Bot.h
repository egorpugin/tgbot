#pragma once

#include "api.h"

namespace tgbot {

class curl_http_client;

/// This object holds other objects specific for this bot instance.
struct TGBOT_API bot {
    bot(const std::string &token);
    virtual ~bot();

    /// returns token for accessing api
    const std::string &get_token() const { return token; }

    /// returns object which can execute Telegram Bot API methods
    const api &get_api() const { return api; }

    /// used for fine tune setup of http connections
    curl_http_client &get_http_client() const { return *httpClient; }

    /// returns last processed update id + 1
    /// Designed to be executed in a loop.
    Integer process_updates(Integer offset = 0, Integer limit = default_update_limit, Integer timeout = default_update_timeout, const Vector<String> &allowed_updates = {});

    /// Calls processUpdates() in a loop.
    /// Starts long poll. After new update will come, this method will parse it
    /// and send it to handleUpdate() method.
    void long_poll(Integer limit = default_update_limit, Integer timeout = default_update_timeout, const Vector<String> &allowed_updates = {});

    ///
    virtual void handle_update(const Update &update) = 0;

    const std::string &get_base_url() const { return base_url; }
    const std::string &get_base_file_url() const { return base_file_url; }
    std::string make_file_url(const std::string &file_path) const;

    int get_default_timeout() const { return 10; }

private:
    std::string token;
    std::unique_ptr<curl_http_client> httpClient;
    api api;
    std::string base_url;
    std::string base_file_url;

    static const int default_update_limit = 100;
    static const int default_update_timeout = 10;
};

}
